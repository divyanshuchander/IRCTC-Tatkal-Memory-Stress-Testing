#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

struct config {
    int seats;
    int requests;
    int workers;
    bool verbose;
    const char *output_dir;
};

int config_parse(int argc, char **argv, struct config *config);
void config_print_usage(const char *program);

#endif
