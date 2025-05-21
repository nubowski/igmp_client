#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include "igmp.h"

#define ANSI_RESET     "\x1b[0m"
#define ANSI_CYAN      "\x1b[36m"
#define ANSI_GREEN     "\x1b[32m"
#define ANSI_YELLOW    "\x1b[33m"
#define ANSI_MAGENTA   "\x1b[35m"
#define ANSI_BOLD      "\x1b[1m"
#define ANSI_RED       "\x1b[31m"

uint16_t checksum(void *data, size_t len);
int parse_int(const char *str, int *out);
int random_uniform(int max);

void print_startup_info(const ClientConfig *cfg, int max_resp_time, int is_v1_enabled);

#endif //UTILS_H
