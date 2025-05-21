#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#include "igmp.h"

uint16_t checksum(void *data, size_t len);
int parse_int(const char *str, int *out);
int random_uniform(int max);

void print_startup_info(const ClientConfig *cfg);

#endif //UTILS_H
