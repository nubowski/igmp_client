#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t checksum(void *data, size_t len);
int parse_int(const char *str, int *out);
int random_uniform(int max);

#endif //UTILS_H
