#ifndef CONFIG_H
#define CONFIG_H

struct config {
    int seats;
    int passengers;
    int requests;
    int workers;
    int queue_capacity;
    const char *output_dir;
};

int config_parse(int argc, char **argv, struct config *config);
void config_print_usage(const char *program);

#endif
