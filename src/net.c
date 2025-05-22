#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>

#include "igmp.h"
#include "igmp_subscribe.h"

#define BUFFER_SIZE 1500           // MTU -> 1500 bytes

void start_igmp_listener() {
    int sock = get_igmp_socket();
    printf("[INFO] Listening for IGMP packets...\n");

    while (1) {
        uint8_t buffer[BUFFER_SIZE];
        ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
        if (len < 0) {
            perror("recv");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        size_t ip_hdr_len = ip->ihl * 4;

        // RFC: Minimum IGMP packet should fit in payload
        // RFC: Ensure IGMP protocol (though we're filtering IPPROTO_IGMP already)
        if (ip->protocol != IPPROTO_IGMP || (size_t)len <= ip_hdr_len) continue;

        // Skip IP header and pass IGMP payload
        uint8_t *igmp_payload = buffer + ip_hdr_len;
        size_t igmp_len = len - ip_hdr_len;
        handle_igmp_packet(igmp_payload, igmp_len);
    }
}
