#ifndef CONFIG_H
#define CONFIG_H

struct config {
    int seats;
    int requests;
    int workers;
    const char *output_dir;
};

int config_parse(int argc, char **argv, struct config *config);
void config_print_usage(const char *program);

#endif
