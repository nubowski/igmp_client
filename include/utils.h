#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

uint16_t checksum(void *data, size_t len);
int parse_int(const char *str, int *out);

#endif //UTILS_H
