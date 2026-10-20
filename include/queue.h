#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#include "app.h"

bool queue_init(struct request_queue *queue, int capacity);
void queue_close(struct request_queue *queue);
bool queue_push(struct request_queue *queue, struct request *request);
struct request *queue_pop(struct request_queue *queue);
void queue_destroy(struct request_queue *queue);

#endif
