#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "app.h"

bool output_prepare(struct app *app);
void output_event(struct app *app, const char *message);
void output_oom(struct app *app, const char *message);
void output_reservation(struct app *app, const struct reservation *reservation);
void output_write_reports(struct app *app);
bool output_start_system_metrics(struct app *app);
void output_stop_system_metrics(struct app *app);
void output_close(struct app *app);

#endif
