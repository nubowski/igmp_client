#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

#define DEFAULT_GROUP "224.0.0.1"
#define INTERFACE "eth0"
#define QUERY_INTERVAL_SEC 2
#define REPEAT_COUNT 5

int main(int argc, char *argv[]) {
    const char *group = DEFAULT_GROUP;
    if (argc >= 2) {
        group = argv[1];
    }

    printf("[TEST] Sending IGMPv2 Query to %s via %s\n", group, INTERFACE);

    for (int i = 0; i < REPEAT_COUNT; i++) {
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
        if (sock < 0) {
            perror("socket");
            return 1;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE, strlen(INTERFACE)) < 0) {
            perror("SO_BINDTODEVICE");
            close(sock);
            return 1;
        }

        struct igmp msg = {0};
        msg.igmp_type = 0x11;
        msg.igmp_code = 20;                                 // Max Resp Time (20 * 100ms = 2s)
        inet_pton(AF_INET, group, &msg.igmp_group);
        msg.igmp_cksum = checksum(&msg, sizeof(msg));

        struct sockaddr_in dst = {0};
        dst.sin_family = AF_INET;
        inet_pton(AF_INET, group, &dst.sin_addr);

        ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dst, sizeof(dst));
        if (sent < 0) {
            perror("sendto");
        } else {
            printf("[TEST] IGMPv2 Query sent to %s via %s\n", group, INTERFACE);
        }

        close(sock);
        sleep(QUERY_INTERVAL_SEC);
    }

    printf("[TEST] Done\n");
    return 0;
}