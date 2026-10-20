#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "app.h"
#include "booking.h"
#include "config.h"
#include "memory.h"
#include "output.h"
#include "queue.h"

int main(int argc, char **argv)
{
    struct app app = {0};
    pthread_t *workers;
    int created = 0;
    int index;
    int parse_result = config_parse(argc, argv, &app.config);

    if (parse_result == 2) return EXIT_SUCCESS;
    if (parse_result == 0) return EXIT_FAILURE;
    if (!booking_init(&app)) return EXIT_FAILURE;
    if (pthread_mutex_init(&app.output_mutex, NULL) != 0) {
        booking_cleanup(&app);
        return EXIT_FAILURE;
    }
    if (!output_prepare(&app)) {
        (void)pthread_mutex_destroy(&app.output_mutex);
        booking_cleanup(&app);
        return EXIT_FAILURE;
    }
    workers = memory_alloc((size_t)app.config.workers * sizeof(*workers));
    if (workers == NULL) {
        output_close(&app);
        booking_cleanup(&app);
        return EXIT_FAILURE;
    }
    (void)clock_gettime(CLOCK_MONOTONIC, &app.metrics.started);
    for (index = 0; index < app.config.workers; index++) {
        if (pthread_create(&workers[index], NULL, booking_worker, &app) != 0) break;
        created++;
    }
    for (index = 0; index < app.config.requests; index++) {
        struct request *request = memory_alloc(sizeof(*request));
        if (request == NULL) {
            (void)pthread_mutex_lock(&app.metrics.mutex);
            app.metrics.completed++;
            app.metrics.failed++;
            app.metrics.oom_failures++;
            (void)pthread_mutex_unlock(&app.metrics.mutex);
            output_event(&app, "request allocation failed: oom");
            continue;
        }
        request->id = index + 1;
        request->passenger_id = (index % app.config.passengers) + 1;
        if (!queue_push(&app.queue, request)) memory_free(request);
        else app.metrics.submitted++;
    }
    queue_close(&app.queue);
    for (index = 0; index < created; index++) (void)pthread_join(workers[index], NULL);
    memory_free(workers);
    output_write_reports(&app);
    (void)printf("Run artifacts: %s\nBookings: %zu successful, %zu failed\nRemaining seats: %d\nPeak tracked bytes: %zu\nOOM failures: %zu\n", app.run_dir, app.metrics.successful, app.metrics.failed, app.train.available, memory_peak_bytes(), memory_failures());
    output_close(&app);
    (void)pthread_mutex_destroy(&app.output_mutex);
    booking_cleanup(&app);
    return EXIT_SUCCESS;
}
