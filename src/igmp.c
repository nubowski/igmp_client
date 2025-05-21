#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>

#include "igmp.h"
#include "fsm.h"
#include "utils.h"

void send_igmp_leave(const char *group_ip, const char *interface) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
        perror("SO_BINDTODEVICE (leave)");
        close(sock);
        return;
    }

    struct igmp msg = {0};
    msg.igmp_type = IGMP_TYPE_LEAVE_GROUP; // 0x17
    msg.igmp_code = 0;
    inet_pton(AF_INET, group_ip, &msg.igmp_group);
    msg.igmp_cksum = checksum(&msg, sizeof(msg));

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    inet_pton(AF_INET, group_ip, &dst.sin_addr);        // RFC !!  leave messages go to 224.0.0.2 (all-routers)

    ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&dst, sizeof(dst));
    if (sent < 0) {
        perror("[ERROR] sendto (leave)");
    } else {
        printf("Sent IGMPv2 Leave to %s via %s\n", group_ip, interface);
    }

    close(sock);
}

void send_igmp_report(const char *group_ip, const char *interface) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (sock < 0) {
        perror("socket");
        return;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
        perror("SO_BINDTODEVICE (report)");
        close(sock);
        return;
    }

    struct igmp msg = {0};
    msg.igmp_type = IGMP_TYPE_MEMBERSHIP_REPORT;
    msg.igmp_code = 0;
    inet_pton(AF_INET, group_ip, &msg.igmp_group);
    msg.igmp_cksum = checksum(&msg, sizeof(msg));

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    inet_pton(AF_INET, group_ip, &dst.sin_addr);

    ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&dst, sizeof(dst));
    if (sent < 0) {
        perror("[ERROR] sendto (report)");
    } else {
        printf("Sent IGMPv2 Report to %s via %s\n", group_ip, interface);
    }

    close(sock);
}

void send_igmp_reports(const ClientConfig *cfg) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // stick to interface
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, cfg->interface, strlen(cfg->interface)) < 0) {
        perror("setsockopt: SO_BINDTODEVICE");
        close(sock);
        exit(1);
    }

    for (int i = 0; i < cfg->group_count; i++) {
        struct igmp msg = {0};
        msg.igmp_type = IGMP_TYPE_MEMBERSHIP_REPORT; // 0x16
        msg.igmp_code = 0;
        inet_pton(AF_INET, cfg->groups[i], &msg.igmp_group);
        msg.igmp_cksum = checksum(&msg, sizeof(msg));

        struct sockaddr_in dst = {0};
        dst.sin_family = AF_INET;
        inet_pton(AF_INET, cfg->groups[i], &dst.sin_addr);

        ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&dst, sizeof(dst));
        if (sent < 0) {
            perror("[ERROR] sendto failed");
        } else {
            printf("Sent IGMPv2 Report to %s\n", cfg->groups[i]);

            GroupInfo* g = find_or_create_group(cfg->groups[i]);
            if (g) {
                handle_event(g, EV_JOIN_GROUP);
            }
        }
    }

    close(sock);
}

// TODO: rework for some readability
void handle_igmp_packet(const uint8_t *data, size_t len) {
    if (len < sizeof(struct igmp)) {
        printf("[RECV] Invalid IGMP packet, too short: %zu bytes\n", len);
        return;
    }

    struct igmp *pkt = (struct igmp *)data;

    if (pkt->igmp_type == IGMP_TYPE_MEMBERSHIP_QUERY) {     // 0x11
        char group[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &pkt->igmp_group, group, sizeof(group));

        // TODO: double check if we could compare `0.0.0.0` and be OK with that
        if (strcmp(group, "0.0.0.0") == 0) {
            printf("[RECV] IGMPv2 General Query\n");
            // apply to all known groups
            for (int i = 0; i < get_group_count(); i++) {
                GroupInfo *g = get_group_at(i);
                handle_event(g, EV_QUERY_RECEIVED);
            }
        } else {
            printf("[RECV] IGMPv2 Group-Specific Query for group %s\n", group);
            GroupInfo *g = find_group(group);
            if (g) {
                handle_event(g, EV_QUERY_RECEIVED);
            }
        }
    }
}