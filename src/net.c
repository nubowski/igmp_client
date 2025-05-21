#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>

#include "igmp.h"

#define BUFFER_SIZE 1500           // MTU -> 1500 bytes

void start_igmp_listener(const ClientConfig *cfg) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (sock < 0) {
        perror("listener socket");
        exit(1);
    }

    // RFC: IGMP messages are received on the same interface used to send them
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, cfg->interface, strlen(cfg->interface)) < 0) {
        perror("SO_BINDTODEVICE (recv)");
        close(sock);
        exit(1);
    }

    printf("[INFO] Listening for IGMP packets on %s...\n", cfg->interface);

    while (1) {
        uint8_t buffer[BUFFER_SIZE];
        ssize_t len = recv(sock, buffer, sizeof(buffer), 0);

        printf("[DEBUG] Got packet len=%zd\n", len);
        fflush(stdout);

        if (len < 0) {
            perror("recv");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        size_t ip_hdr_len = ip->ihl * 4;

        // TODO: try to work around with safy cozy cast checks ~ if (len > 0) => size_t len_u = (size_t)len
        // RFC: Ensure IGMP protocol (though we're filtering IPPROTO_IGMP already)
        if (ip->protocol != IPPROTO_IGMP) continue;

        // RFC: Minimum IGMP packet should fit in payload
        if ((size_t)len <= ip_hdr_len) continue;                // its always positive i guess atm

        // Skip IP header and pass IGMP payload
        uint8_t *igmp_payload = buffer + ip_hdr_len;
        size_t igmp_len = len - ip_hdr_len;

        handle_igmp_packet(igmp_payload, igmp_len);
    }

    close(sock);
}
