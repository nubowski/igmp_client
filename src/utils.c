#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "utils.h"
#include "fsm.h"

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

// RFC: using a delay value chosen uniformly from the interval (0, Max Response Time]
int random_uniform(int max) {
    if (max <= 1) return 1;
    double fraction = (double)rand() / ((double)RAND_MAX + 1);
    int delay = (int)(fraction * max);
    return delay == 0 ? 1 : delay;
}

void print_startup_info(const ClientConfig *cfg, int max_resp_time, int is_v1_enabled) {
    printf(ANSI_BOLD ANSI_CYAN "\n╔═══════════════════════════════════════════════╗\n");
    printf("║               IGMPv2 Client Started           ║\n");
    printf("╠═══════════════════════════════════════════════╣\n" ANSI_RESET);

    printf(ANSI_YELLOW " Interface       " ANSI_RESET ": %s\n", cfg->interface);

    printf(ANSI_YELLOW " Multicast groups" ANSI_RESET ":\n");
    for (int i = 0; i < cfg->group_count; i++) {
        printf("   " ANSI_GREEN "•" ANSI_RESET " %s\n", cfg->groups[i]);
    }

    printf(ANSI_YELLOW " Max Resp Time   " ANSI_RESET ": %d ms\n", max_resp_time);
    printf(ANSI_YELLOW " IGMPv1 Suppress " ANSI_RESET ": %s\n",
           is_v1_enabled ? ANSI_GREEN "ENABLED" ANSI_RESET : ANSI_MAGENTA "DISABLED" ANSI_RESET);

    printf(ANSI_BOLD ANSI_CYAN "╚═══════════════════════════════════════════════╝\n\n" ANSI_RESET);
}