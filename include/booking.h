#ifndef BOOKING_H
#define BOOKING_H

#include <stdbool.h>

#include "app.h"

bool booking_init(struct app *app);
bool booking_process(struct app *app, const struct request *request);
void *booking_worker(void *argument);
void booking_cleanup(struct app *app);

#endif
