#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "booking.h"
#include "memory.h"
#include "output.h"
#include "queue.h"

static uint64_t elapsed_ns(struct timespec start, struct timespec end)
{
    return (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL + (uint64_t)(end.tv_nsec - start.tv_nsec);
}

static void result_metrics(struct app *app, bool success, bool oom, uint64_t latency)
{
    (void)pthread_mutex_lock(&app->metrics.mutex);
    app->metrics.completed++;
    app->metrics.active--;
    app->metrics.failed += success ? 0U : 1U;
    app->metrics.successful += success ? 1U : 0U;
    app->metrics.oom_failures += oom ? 1U : 0U;
    app->metrics.latency_total_ns += latency;
    if (latency > app->metrics.latency_max_ns) app->metrics.latency_max_ns = latency;
    (void)pthread_mutex_unlock(&app->metrics.mutex);
}

static int seat_take(struct train *train)
{
    int index;
    int number = -1;
    (void)pthread_mutex_lock(&train->mutex);
    for (index = 0; index < train->seat_count; index++) {
        if (!train->seats[index].allocated) {
            train->seats[index].allocated = true;
            train->available--;
            number = train->seats[index].number;
            break;
        }
    }
    (void)pthread_mutex_unlock(&train->mutex);
    return number;
}

static void seat_release(struct train *train, int number)
{
    int index;
    (void)pthread_mutex_lock(&train->mutex);
    for (index = 0; index < train->seat_count; index++) {
        if (train->seats[index].number == number && train->seats[index].allocated) {
            train->seats[index].allocated = false;
            train->available++;
            break;
        }
    }
    (void)pthread_mutex_unlock(&train->mutex);
}

bool booking_process(struct app *app, const struct request *request)
{
    struct timespec start;
    struct timespec finish;
    struct reservation *reservation;
    int seat;
    uint64_t latency;
    (void)clock_gettime(CLOCK_MONOTONIC, &start);
    (void)pthread_mutex_lock(&app->metrics.mutex);
    app->metrics.active++;
    if (app->metrics.active > app->metrics.peak_active) app->metrics.peak_active = app->metrics.active;
    (void)pthread_mutex_unlock(&app->metrics.mutex);
    seat = seat_take(&app->train);
    if (seat < 0) {
        char message[128];
        (void)snprintf(message, sizeof(message), "request=%" PRId64 " result=failed reason=no_seat", request->id);
        output_event(app, message);
        (void)clock_gettime(CLOCK_MONOTONIC, &finish);
        result_metrics(app, false, false, elapsed_ns(start, finish));
        return false;
    }
    reservation = memory_alloc(sizeof(*reservation));
    if (reservation == NULL) {
        char message[128];
        seat_release(&app->train, seat);
        (void)snprintf(message, sizeof(message), "request=%" PRId64 " result=failed reason=oom seat_rolled_back=%d", request->id, seat);
        output_event(app, message);
        (void)clock_gettime(CLOCK_MONOTONIC, &finish);
        result_metrics(app, false, true, elapsed_ns(start, finish));
        return false;
    }
    reservation->id = request->id;
    reservation->request_id = request->id;
    reservation->passenger_id = request->passenger_id;
    reservation->seat_number = seat;
    reservation->next = NULL;
    output_reservation(app, reservation);
    (void)pthread_mutex_lock(&app->reservations.mutex);
    if (app->reservations.tail == NULL) app->reservations.head = reservation;
    else app->reservations.tail->next = reservation;
    app->reservations.tail = reservation;
    app->reservations.count++;
    (void)pthread_mutex_unlock(&app->reservations.mutex);
    {
        char message[128];
        (void)snprintf(message, sizeof(message), "request=%" PRId64 " result=confirmed seat=%d", request->id, seat);
        output_event(app, message);
    }
    (void)clock_gettime(CLOCK_MONOTONIC, &finish);
    latency = elapsed_ns(start, finish);
    result_metrics(app, true, false, latency);
    return true;
}

void *booking_worker(void *argument)
{
    struct app *app = argument;
    struct request *request;
    while ((request = queue_pop(&app->queue)) != NULL) {
        (void)booking_process(app, request);
        memory_free(request);
    }
    return NULL;
}

bool booking_init(struct app *app)
{
    int index;
    app->passengers = memory_alloc((size_t)app->config.passengers * sizeof(*app->passengers));
    app->train.seats = memory_alloc((size_t)app->config.seats * sizeof(*app->train.seats));
    if (app->passengers == NULL || app->train.seats == NULL) {
        memory_free(app->train.seats);
        memory_free(app->passengers);
        return false;
    }
    for (index = 0; index < app->config.passengers; index++) {
        app->passengers[index].id = index + 1;
        app->passengers[index].age = 18 + (index % 60);
    }
    for (index = 0; index < app->config.seats; index++) {
        app->train.seats[index].number = index + 1;
        app->train.seats[index].allocated = false;
    }
    app->train.seat_count = app->config.seats;
    app->train.available = app->config.seats;
    if (pthread_mutex_init(&app->train.mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&app->reservations.mutex, NULL) != 0) {
        (void)pthread_mutex_destroy(&app->train.mutex);
        return false;
    }
    if (pthread_mutex_init(&app->metrics.mutex, NULL) != 0) {
        (void)pthread_mutex_destroy(&app->reservations.mutex);
        (void)pthread_mutex_destroy(&app->train.mutex);
        return false;
    }
    if (!queue_init(&app->queue, app->config.queue_capacity)) {
        (void)pthread_mutex_destroy(&app->metrics.mutex);
        (void)pthread_mutex_destroy(&app->reservations.mutex);
        (void)pthread_mutex_destroy(&app->train.mutex);
        memory_free(app->train.seats);
        memory_free(app->passengers);
        app->train.seats = NULL;
        app->passengers = NULL;
        return false;
    }
    return true;
}

void booking_cleanup(struct app *app)
{
    struct reservation *current = app->reservations.head;
    while (current != NULL) {
        struct reservation *next = current->next;
        memory_free(current);
        current = next;
    }
    queue_destroy(&app->queue);
    (void)pthread_mutex_destroy(&app->metrics.mutex);
    (void)pthread_mutex_destroy(&app->reservations.mutex);
    (void)pthread_mutex_destroy(&app->train.mutex);
    memory_free(app->train.seats);
    memory_free(app->passengers);
}
