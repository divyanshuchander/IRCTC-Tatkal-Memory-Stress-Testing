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
    if (app->memory_log == NULL || app->oom_log == NULL) return false;
    return true;
}

void output_event(struct app *app, const char *message)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->events, "%s\n", message);
    (void)fflush(app->events);
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_oom(struct app *app, const char *message)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->oom_log, "%s\n", message);
    (void)fflush(app->oom_log);
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_reservation(struct app *app, const struct reservation *reservation)
{
    (void)pthread_mutex_lock(&app->output_mutex);
    (void)fprintf(app->reservations_file, "%" PRId64 ",%" PRId64 ",%d,%d,CONFIRMED\n", reservation->id, reservation->request_id, reservation->passenger_id, reservation->seat_number);
    (void)fflush(app->reservations_file);
    (void)pthread_mutex_unlock(&app->output_mutex);
}

void output_write_reports(struct app *app)
{
    char path[640];
    FILE *file;
    uint64_t duration_ns;
    double seconds;
    double average_ms;
    char metrics_dir[640];
    if (clock_gettime(CLOCK_MONOTONIC, &app->metrics.finished) != 0) return;
    duration_ns = (uint64_t)(app->metrics.finished.tv_sec - app->metrics.started.tv_sec) * 1000000000ULL + (uint64_t)(app->metrics.finished.tv_nsec - app->metrics.started.tv_nsec);
    seconds = (double)duration_ns / 1000000000.0;
    average_ms = app->metrics.completed == 0 ? 0.0 : (double)app->metrics.latency_total_ns / (double)app->metrics.completed / 1000000.0;
    if (snprintf(metrics_dir, sizeof(metrics_dir), "%s/metrics", app->run_dir) >= (int)sizeof(metrics_dir)) return;
    if (snprintf(path, sizeof(path), "%s/metrics.csv", metrics_dir) < (int)sizeof(path) && (file = fopen(path, "w")) != NULL) {
        (void)fprintf(file, "metric,value\nrequests,%d\nsubmitted,%zu\ncompleted,%zu\nsuccessful,%zu\nfailed,%zu\noom_failures,%zu\nremaining_seats,%d\nreservations,%zu\nduration_seconds,%.6f\naverage_latency_ms,%.6f\nmax_latency_ms,%.6f\ncurrent_bytes,%zu\npeak_bytes,%zu\noutstanding_allocations,%zu\n", app->config.requests, app->metrics.submitted, app->metrics.completed, app->metrics.successful, app->metrics.failed, app->metrics.oom_failures, app->train.available, app->reservations.count, seconds, average_ms, (double)app->metrics.latency_max_ns / 1000000.0, memory_current_bytes(), memory_peak_bytes(), memory_outstanding());
        (void)fclose(file);
    }
    (void)fprintf(app->memory_log, "current_bytes=%zu peak_bytes=%zu allocations=%zu frees=%zu outstanding_allocations=%zu\n", memory_current_bytes(), memory_peak_bytes(), memory_allocations(), memory_frees(), memory_outstanding());
    (void)fflush(app->memory_log);
    if (snprintf(path, sizeof(path), "%s/summary.txt", app->run_dir) < (int)sizeof(path) && (file = fopen(path, "w")) != NULL) {
        (void)fprintf(file, "IRCTC Tatkal memory stress run\nrequests: %d\ncompleted: %zu\nsuccessful: %zu\nfailed: %zu\nremaining seats: %d\nduration seconds: %.6f\naverage latency ms: %.6f\npeak tracked bytes: %zu\noutstanding allocations: %zu\n", app->config.requests, app->metrics.completed, app->metrics.successful, app->metrics.failed, app->train.available, seconds, average_ms, memory_peak_bytes(), memory_outstanding());
        (void)fclose(file);
    }
}

void output_close(struct app *app)
{
    if (app->events != NULL) (void)fclose(app->events);
    if (app->reservations_file != NULL) (void)fclose(app->reservations_file);
    if (app->memory_log != NULL) (void)fclose(app->memory_log);
    if (app->oom_log != NULL) (void)fclose(app->oom_log);
}
