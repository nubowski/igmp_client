#ifndef IGMP_H
#define IGMP_H

#define MAX_GROUPS 64

typedef struct {
    char interface[32];
    int group_count;
    char groups[MAX_GROUPS][16];
} ClientConfig;

void print_config(const ClientConfig *cfg);

#endif //IGMP_H
