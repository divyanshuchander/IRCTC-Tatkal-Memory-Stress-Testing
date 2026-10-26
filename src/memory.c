#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "memory.h"

struct header { size_t size; };
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static size_t current_bytes;
static size_t peak_bytes;
static size_t allocations;
static size_t frees;
static size_t failures;
static size_t attempts;
static long fail_after = -1;

void *memory_alloc(size_t size)
{
    struct header *header;
    size_t total;

    if (size == 0 || size > SIZE_MAX - sizeof(*header)) return NULL;
    (void)pthread_mutex_lock(&lock);
    if (fail_after >= 0 && attempts >= (size_t)fail_after) {
        failures++;
        attempts++;
        (void)pthread_mutex_unlock(&lock);
        return NULL;
    }
    attempts++;
    (void)pthread_mutex_unlock(&lock);
    total = size + sizeof(*header);
    header = malloc(total);
    if (header == NULL) {
        (void)pthread_mutex_lock(&lock);
        failures++;
        (void)pthread_mutex_unlock(&lock);
        return NULL;
    }
    header->size = size;
    (void)pthread_mutex_lock(&lock);
    current_bytes += size;
    if (current_bytes > peak_bytes) peak_bytes = current_bytes;
    allocations++;
    (void)pthread_mutex_unlock(&lock);
    return header + 1;
}

void memory_free(void *pointer)
{
    struct header *header;
    if (pointer == NULL) return;
    header = ((struct header *)pointer) - 1;
    (void)pthread_mutex_lock(&lock);
    current_bytes -= header->size;
    frees++;
    (void)pthread_mutex_unlock(&lock);
    free(header);
}

void memory_fail_after(long allocation_number)
{
    (void)pthread_mutex_lock(&lock);
    fail_after = allocation_number;
    (void)pthread_mutex_unlock(&lock);
}

size_t memory_current_bytes(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = current_bytes;
    (void)pthread_mutex_unlock(&lock);
    return value;
}

size_t memory_peak_bytes(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = peak_bytes;
    (void)pthread_mutex_unlock(&lock);
    return value;
}

size_t memory_allocations(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = allocations;
    (void)pthread_mutex_unlock(&lock);
    return value;
}

size_t memory_frees(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = frees;
    (void)pthread_mutex_unlock(&lock);
    return value;
}

size_t memory_failures(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = failures;
    (void)pthread_mutex_unlock(&lock);
    return value;
}

size_t memory_outstanding(void)
{
    size_t value;
    (void)pthread_mutex_lock(&lock);
    value = allocations - frees;
    (void)pthread_mutex_unlock(&lock);
    return value;
}
