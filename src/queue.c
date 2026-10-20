#include <pthread.h>

#include "memory.h"
#include "queue.h"

bool queue_init(struct request_queue *queue, int capacity)
{
    queue->items = memory_alloc((size_t)capacity * sizeof(*queue->items));
    if (queue->items == NULL) return false;
    queue->capacity = capacity;
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        memory_free(queue->items);
        queue->items = NULL;
        return false;
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        (void)pthread_mutex_destroy(&queue->mutex);
        memory_free(queue->items);
        queue->items = NULL;
        return false;
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        (void)pthread_cond_destroy(&queue->not_empty);
        (void)pthread_mutex_destroy(&queue->mutex);
        memory_free(queue->items);
        queue->items = NULL;
        return false;
    }
    return true;
}

bool queue_push(struct request_queue *queue, struct request *request)
{
    (void)pthread_mutex_lock(&queue->mutex);
    while (queue->count == queue->capacity && !queue->closed) (void)pthread_cond_wait(&queue->not_full, &queue->mutex);
    if (queue->closed) {
        (void)pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    queue->items[queue->tail] = request;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    (void)pthread_cond_signal(&queue->not_empty);
    (void)pthread_mutex_unlock(&queue->mutex);
    return true;
}

struct request *queue_pop(struct request_queue *queue)
{
    struct request *request;
    (void)pthread_mutex_lock(&queue->mutex);
    while (queue->count == 0 && !queue->closed) (void)pthread_cond_wait(&queue->not_empty, &queue->mutex);
    if (queue->count == 0 && queue->closed) {
        (void)pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    request = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    (void)pthread_cond_signal(&queue->not_full);
    (void)pthread_mutex_unlock(&queue->mutex);
    return request;
}

void queue_close(struct request_queue *queue)
{
    (void)pthread_mutex_lock(&queue->mutex);
    queue->closed = true;
    (void)pthread_cond_broadcast(&queue->not_empty);
    (void)pthread_cond_broadcast(&queue->not_full);
    (void)pthread_mutex_unlock(&queue->mutex);
}

void queue_destroy(struct request_queue *queue)
{
    memory_free(queue->items);
    (void)pthread_cond_destroy(&queue->not_empty);
    (void)pthread_cond_destroy(&queue->not_full);
    (void)pthread_mutex_destroy(&queue->mutex);
}
