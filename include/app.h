#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "config.h"

struct seat {
    int number;
    bool allocated;
};

struct train {
    struct seat *seats;
    int seat_count;
    int available;
    pthread_mutex_t mutex;
};

struct request {
    int64_t id;
    int passenger_id;
};

struct reservation {
    int64_t id;
    int64_t request_id;
    int passenger_id;
    int seat_number;
    struct reservation *next;
};

struct reservation_store {
    struct reservation *head;
    struct reservation *tail;
    size_t count;
    pthread_mutex_t mutex;
};

struct request_queue {
    struct request **items;
    int capacity;
    int head;
    int tail;
    int count;
    bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

struct metrics {
    size_t submitted;
    size_t completed;
    size_t successful;
    size_t failed;
    size_t oom_failures;
    uint64_t latency_total_ns;
    uint64_t latency_max_ns;
    struct timespec started;
    struct timespec finished;
    pthread_mutex_t mutex;
};

struct app {
    struct config config;
    struct train train;
    struct reservation_store reservations;
    struct request_queue queue;
    struct metrics metrics;
    pthread_mutex_t output_mutex;
    FILE *events;
    FILE *reservations_file;
    FILE *memory_log;
    FILE *oom_log;
    char run_dir[512];
};

#endif
