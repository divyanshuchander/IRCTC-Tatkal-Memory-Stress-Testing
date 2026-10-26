#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory.h"

#define MAX_VALUE 1000000L

void config_print_usage(const char *program)
{
    printf("Usage: %s [--seats N] [--requests N] [--workers N] [--fail-after N] [--verbose] [--output-dir PATH]\n", program);
}

static int positive_value(const char *text, int *value)
{
    char *end;
    long parsed;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < 1 || parsed > MAX_VALUE) return 0;
    *value = (int)parsed;
    return 1;
}

int config_parse(int argc, char **argv, struct config *config)
{
    int index;
    int value;
    config->seats = 10;
    config->requests = 100;
    config->workers = 4;
    config->verbose = false;
    config->output_dir = "artifacts";
    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--help") == 0) {
            config_print_usage(argv[0]);
            return 2;
        }
        if (strcmp(argv[index], "--verbose") == 0) {
            config->verbose = true;
            continue;
        }
        if (index + 1 >= argc) return 0;
        if (strcmp(argv[index], "--output-dir") == 0) {
            config->output_dir = argv[++index];
        } else if (strcmp(argv[index], "--fail-after") == 0) {
            char *end;
            long parsed = strtol(argv[++index], &end, 10);
            if (end == argv[index] || *end != '\0' || parsed < 0) return 0;
            memory_fail_after(parsed);
        } else if (strcmp(argv[index], "--seats") == 0 || strcmp(argv[index], "--requests") == 0 || strcmp(argv[index], "--workers") == 0) {
            if (!positive_value(argv[++index], &value)) return 0;
            if (strcmp(argv[index - 1], "--seats") == 0) config->seats = value;
            else if (strcmp(argv[index - 1], "--requests") == 0) config->requests = value;
            else config->workers = value;
        } else return 0;
    }
    return 1;
}
