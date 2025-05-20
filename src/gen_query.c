#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

#define GROUP "224.0.0.1"
#define INTERFACE "eth0"

int main() {
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
    msg.igmp_code = 20;    // Max Resp Time (20 * 100ms = 2s)
    inet_pton(AF_INET, GROUP, &msg.igmp_group);
    msg.igmp_cksum = checksum(&msg, sizeof(msg));

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    inet_pton(AF_INET, GROUP, &dst.sin_addr);

    ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dst, sizeof(dst));
    if (sent < 0) {
        perror("sendto");
    } else {
        printf("[TEST] IGMPv2 Query sent to %s via %s\n", GROUP, INTERFACE);
    }

    close(sock);
    return 0;
}