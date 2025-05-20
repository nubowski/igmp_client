#include <stdio.h>

#include "igmp.h"

void print_config(const ClientConfig *cfg) {
    printf("Interface: %s\n", cfg->interface);
    printf("Groups:\n");

    for (int i = 0; i < cfg->group_count; i++) {
        printf(" - %s\n", cfg->groups[i]);
    }
}