#ifndef UTILS_H
#define UTILS_H

#include "fsm.h"

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
int is_valid_ipv4(const char *ip);
const char* state_to_str(GroupState s);

#endif //UTILS_H
