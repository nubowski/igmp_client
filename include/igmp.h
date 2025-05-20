#ifndef IGMP_H
#define IGMP_H

#include <stdint.h>

#define MAX_GROUPS 64

#define IGMP_TYPE_MEMBERSHIP_QUERY 0x11
#define IGMP_TYPE_MEMBERSHIP_REPORT 0x16
#define IGMP_TYPE_LEAVE_GROUP 0x17


typedef struct {
    char interface[32];
    int group_count;
    char groups[MAX_GROUPS][16];
} ClientConfig;

void print_config(const ClientConfig *cfg);                  // TODO: drop it outside to cli.h
void send_igmp_reports(const ClientConfig *cfg);
void start_igmp_listener(const ClientConfig *cfg);           // TODO: drop it outside to net.h
void handle_igmp_packet(const uint8_t *data, size_t len);

#endif //IGMP_H
