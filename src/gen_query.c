#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>
#include <unistd.h>

#define GROUP "224.0.0.1"
#define INTERFACE "eth0"

static uint16_t checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t* ptr = data;

    for (; len > 1; len -= 2)
        sum += *ptr++;

    if (len == 1)
        sum += *(uint8_t*)ptr;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

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