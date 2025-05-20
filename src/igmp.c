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

// chke sum (-> RFC)
static uint16_t checksum(void *data, size_t len) {
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
            perror("sendto");
        } else {
            printf("Sent IGMPv2 Report to %s\n", cfg->groups[i]);
        }
    }

    close(sock);
}

void handle_igmp_packet(const uint8_t *data, size_t len) {
    if (len < sizeof(struct igmp)) {
        printf("[RECV] Invalid IGMP packet, too short: %zu bytes\n", len);
        return;
    }

    struct igmp *pkt = (struct igmp *)data;

    if (pkt->igmp_type == IGMP_TYPE_MEMBERSHIP_QUERY) {     // 0x11
        char group[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &pkt->igmp_group, group, sizeof(group));
        printf("[RECV] IGMPv2 Query received for group %s\n", group);
    }
}