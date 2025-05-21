#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "utils.h"


uint16_t checksum(void *data, size_t len) {
    uint16_t sum = 0;
    uint16_t *ptr = data;

    for (; len > 1; len -= 2) {
        sum += *ptr++;
    }

    if (len == 1) {
        sum += *(uint8_t *)ptr;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ~sum;
}

int parse_int(const char *str, int *out) {
    if (!str || !out) return 0;

    // could be checked without including errno.h, manually by checking all possibles, but I dont know how to check invalid input vs type
    char *endptr = NULL;
    errno = 0;

    long val = strtol(str, &endptr, 10);

    // limits theoretically could be passed by errno
    if (val < INT_MIN || val > INT_MAX) return 0;

    if (errno != 0 || *endptr != '\0') {
        return 0;
    }

    *out = (int)val;
    return 1;
}