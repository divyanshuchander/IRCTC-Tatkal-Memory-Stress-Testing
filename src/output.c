#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "memory.h"
#include "output.h"

static void *system_metrics_worker(void *argument)
{
    struct app *app = argument;
    struct timespec delay = {0, 500000000L};
    unsigned long elapsed_ms = 0;

    while (true) {
        FILE *status = fopen("/proc/self/status", "r");
        unsigned long rss_kb = 0;
        unsigned long virtual_kb = 0;
        unsigned long threads = 0;

        if (status != NULL) {
            char line[128];

            while (fgets(line, sizeof(line), status) != NULL) {
                if (sscanf(line, "VmRSS: %lu kB", &rss_kb) == 1) continue;
                if (sscanf(line, "VmSize: %lu kB", &virtual_kb) == 1) continue;
                (void)sscanf(line, "Threads: %lu", &threads);
            }
            (void)fclose(status);
        }
        (void)pthread_mutex_lock(&app->output_mutex);
        if (app->system_metrics_stop) {
            (void)pthread_mutex_unlock(&app->output_mutex);
            break;
        }
        (void)fprintf(app->system_metrics_file, "%lu,%lu,%lu,%lu,%zu,%zu\n",
                      elapsed_ms, rss_kb, virtual_kb, threads,
                      memory_current_bytes(), memory_peak_bytes());
        (void)fflush(app->system_metrics_file);
        if (app->config.verbose) {
            (void)printf("[metrics] elapsed=%lums rss=%lukB tracked=%zuB peak=%zuB threads=%lu\n",
                         elapsed_ms, rss_kb, memory_current_bytes(),
                         memory_peak_bytes(), threads);
            (void)fflush(stdout);
        }
        (void)pthread_mutex_unlock(&app->output_mutex);
        (void)nanosleep(&delay, NULL);
        elapsed_ms += 500;
    }
    return NULL;
}

static bool make_dirs(const char *path)
{
    char buffer[512];
    size_t index;
    size_t length = strlen(path);
    if (length == 0 || length >= sizeof(buffer)) return false;
    (void)memcpy(buffer, path, length + 1);
    for (index = 1; index <= length; index++) {
        if (buffer[index] == '/' || buffer[index] == '\0') {
            char separator = buffer[index];
            buffer[index] = '\0';
            if (buffer[0] != '\0' && mkdir(buffer, 0755) != 0 && errno != EEXIST) return false;
            buffer[index] = separator;
        }
    }
    return true;
}

bool output_prepare(struct app *app)
{
    time_t now = time(NULL);
    struct tm local_time;
    char timestamp[32];
    char path[640];
    char metrics_dir[640];
    if (!make_dirs(app->config.output_dir)) return false;
    if (localtime_r(&now, &local_time) == NULL || strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &local_time) == 0) return false;
    if (snprintf(app->run_dir, sizeof(app->run_dir), "%s/run_%s_%ld", app->config.output_dir, timestamp, (long)getpid()) >= (int)sizeof(app->run_dir)) return false;
    if (!make_dirs(app->run_dir)) return false;
    if (snprintf(metrics_dir, sizeof(metrics_dir), "%s/metrics", app->run_dir) >= (int)sizeof(metrics_dir) || !make_dirs(metrics_dir)) return false;
    if (snprintf(path, sizeof(path), "%s/events.log", app->run_dir) >= (int)sizeof(path)) return false;
    app->events = fopen(path, "w");
    if (snprintf(path, sizeof(path), "%s/reservations.csv", app->run_dir) >= (int)sizeof(path)) return false;
    app->reservations_file = fopen(path, "w");
    if (app->events == NULL || app->reservations_file == NULL) return false;
    (void)fprintf(app->reservations_file, "reservation_id,request_id,passenger_id,seat_number,status\n");
    if (snprintf(path, sizeof(path), "%s/memory.log", metrics_dir) >= (int)sizeof(path)) return false;
    app->memory_log = fopen(path, "w");
    if (snprintf(path, sizeof(path), "%s/oom.log", metrics_dir) >= (int)sizeof(path)) return false;
    app->oom_log = fopen(path, "w");
    if (snprintf(path, sizeof(path), "%s/system.csv", metrics_dir) >= (int)sizeof(path)) return false;
    app->system_metrics_file = fopen(path, "w");
    if (app->memory_log == NULL || app->oom_log == NULL || app->system_metrics_file == NULL) return false;
    (void)fprintf(app->system_metrics_file, "elapsed_ms,rss_kb,vmsize_kb,threads,tracked_bytes,peak_tracked_bytes\n");
    (void)fflush(app->system_metrics_file);
    return true;
}

void output_event(struct app *app, const char *message)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->events, "%s\n", message);
    if (app->config.verbose) {
        (void)printf("%s\n", message);
        (void)fflush(stdout);
    }
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_oom(struct app *app, const char *message)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->oom_log, "%s\n", message);
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_reservation(struct app *app, const struct reservation *reservation)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->reservations_file, "%" PRId64 ",%" PRId64 ",%d,%d,CONFIRMED\n", reservation->id, reservation->request_id, reservation->passenger_id, reservation->seat_number);
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_write_reports(struct app *app)
{
    char path[640];
    FILE *file;
    uint64_t duration_ns;
    double seconds;
    double average_ms;
    double max_ms;
    double throughput;
    double success_rate;
    double failure_rate;
    char metrics_dir[640];
    if (clock_gettime(CLOCK_MONOTONIC, &app->metrics.finished) != 0) return;
    duration_ns = (uint64_t)(app->metrics.finished.tv_sec - app->metrics.started.tv_sec) * 1000000000ULL;
    if (app->metrics.finished.tv_nsec >= app->metrics.started.tv_nsec) {
        duration_ns += (uint64_t)(app->metrics.finished.tv_nsec - app->metrics.started.tv_nsec);
    } else {
        duration_ns -= 1000000000ULL;
        duration_ns += (uint64_t)(app->metrics.finished.tv_nsec + 1000000000L - app->metrics.started.tv_nsec);
    }
    seconds = (double)duration_ns / 1000000000.0;
    average_ms = app->metrics.completed == 0 ? 0.0 : (double)app->metrics.latency_total_ns / (double)app->metrics.completed / 1000000.0;
    max_ms = (double)app->metrics.latency_max_ns / 1000000.0;
    throughput = seconds > 0.0 ? (double)app->metrics.completed / seconds : 0.0;
    success_rate = app->metrics.completed == 0 ? 0.0 : (double)app->metrics.successful * 100.0 / (double)app->metrics.completed;
    failure_rate = app->metrics.completed == 0 ? 0.0 : (double)app->metrics.failed * 100.0 / (double)app->metrics.completed;
    if (snprintf(metrics_dir, sizeof(metrics_dir), "%s/metrics", app->run_dir) >= (int)sizeof(metrics_dir)) return;
    if (snprintf(path, sizeof(path), "%s/metrics.csv", metrics_dir) < (int)sizeof(path) && (file = fopen(path, "w")) != NULL) {
        (void)fprintf(file, "metric,value\nseats,%d\nworkers,%d\nrequests,%d\nverbose,%d\nqueued,%zu\nnot_queued,%zu\ncompleted,%zu\nsuccessful,%zu\nfailed,%zu\noom_failures,%zu\nrequest_oom_failures,%zu\nreservation_oom_failures,%zu\nseat_rollbacks,%zu\nunprocessed,%zu\nremaining_seats,%d\nreservations,%zu\nduration_seconds,%.6f\nthroughput_requests_per_second,%.6f\nsuccess_rate_percent,%.6f\nfailure_rate_percent,%.6f\naverage_latency_ms,%.6f\nmax_latency_ms,%.6f\nallocation_count,%zu\nfree_count,%zu\nallocation_failures,%zu\ncurrent_bytes,%zu\npeak_bytes,%zu\noutstanding_allocations,%zu\n", app->config.seats, app->config.workers, app->config.requests, app->config.verbose, app->metrics.submitted, (size_t)app->config.requests - app->metrics.submitted, app->metrics.completed, app->metrics.successful, app->metrics.failed, app->metrics.oom_failures, app->metrics.request_oom_failures, app->metrics.reservation_oom_failures, app->metrics.seat_rollbacks, (size_t)app->config.requests - app->metrics.completed, app->train.available, app->reservations.count, seconds, throughput, success_rate, failure_rate, average_ms, max_ms, memory_allocations(), memory_frees(), memory_failures(), memory_current_bytes(), memory_peak_bytes(), memory_outstanding());
        (void)fclose(file);
    }
    (void)fprintf(app->memory_log, "current_bytes=%zu peak_bytes=%zu allocations=%zu frees=%zu allocation_failures=%zu outstanding_allocations=%zu\n", memory_current_bytes(), memory_peak_bytes(), memory_allocations(), memory_frees(), memory_failures(), memory_outstanding());
    (void)fflush(app->memory_log);
    if (snprintf(path, sizeof(path), "%s/summary.txt", app->run_dir) < (int)sizeof(path) && (file = fopen(path, "w")) != NULL) {
        (void)fprintf(file, "IRCTC Tatkal memory stress run\n\nConfiguration\n seats: %d\n workers: %d\n requested requests: %d\n output directory: %s\n\nRequest results\n queued: %zu\n not queued: %zu\n completed: %zu\n unprocessed: %zu\n successful: %zu (%.2f%%)\n failed: %zu (%.2f%%)\n OOM failures: %zu\n\nSeat results\n reservations: %zu\n remaining seats: %d\n\nPerformance\n duration seconds: %.6f\n throughput requests/second: %.2f\n average latency ms: %.6f\n maximum latency ms: %.6f\n\nMemory\n current tracked bytes: %zu\n peak tracked bytes: %zu\n allocations: %zu\n frees: %zu\n allocation failures: %zu\n outstanding allocations: %zu\n\nReports\n events: events.log\n reservations: reservations.csv\n metrics: metrics/metrics.csv\n memory log: metrics/memory.log\n OOM log: metrics/oom.log\n", app->config.seats, app->config.workers, app->config.requests, app->config.output_dir, app->metrics.submitted, (size_t)app->config.requests - app->metrics.submitted, app->metrics.completed, (size_t)app->config.requests - app->metrics.completed, app->metrics.successful, success_rate, app->metrics.failed, failure_rate, app->metrics.oom_failures, app->reservations.count, app->train.available, seconds, throughput, average_ms, max_ms, memory_current_bytes(), memory_peak_bytes(), memory_allocations(), memory_frees(), memory_failures(), memory_outstanding());
        (void)fclose(file);
    }
}

bool output_start_system_metrics(struct app *app)
{
    if (app->system_metrics_file == NULL) return false;
    app->system_metrics_stop = false;
    if (pthread_create(&app->system_metrics_thread, NULL, system_metrics_worker, app) != 0) return false;
    app->system_metrics_active = true;
    return true;
}

void output_stop_system_metrics(struct app *app)
{
    if (!app->system_metrics_active) return;
    (void)pthread_mutex_lock(&app->output_mutex);
    app->system_metrics_stop = true;
    (void)pthread_mutex_unlock(&app->output_mutex);
    (void)pthread_join(app->system_metrics_thread, NULL);
    app->system_metrics_active = false;
}

void output_close(struct app *app)
{
    output_stop_system_metrics(app);
    (void)pthread_mutex_lock(&app->output_mutex);
    if (app->events != NULL) (void)fclose(app->events);
    if (app->reservations_file != NULL) (void)fclose(app->reservations_file);
    if (app->memory_log != NULL) (void)fclose(app->memory_log);
    if (app->oom_log != NULL) (void)fclose(app->oom_log);
    if (app->system_metrics_file != NULL) (void)fclose(app->system_metrics_file);
    (void)pthread_mutex_unlock(&app->output_mutex);
}
