#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_GROUP "0.0.0.0"
#define INTERFACE "eth0"
#define QUERY_INTERVAL_SEC 5
#define REPEAT_COUNT 4
#define RANDOM 1

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

int main(int argc, char *argv[]) {
    int group_count = argc > 1 ? argc - 1 : 1;
    const char **groups = (const char **)malloc(group_count * sizeof(char *));
    if (!groups) {
        perror("malloc");
        return 1;
    }

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            groups[i - 1] = argv[i];
        }
    } else {
        groups[0] = DEFAULT_GROUP;
    }

    srand(time(NULL));

    for (int i = 0; i < REPEAT_COUNT; i++) {
        int g = 0;

#if RANDOM
        g = rand() % group_count;
#else
        g = i % group_count;
#endif

        const char *group = groups[g];

        struct in_addr group_addr = {0};
        if (inet_pton(AF_INET, group, &group_addr) != 1) {
            fprintf(stderr, "[TEST] Invalid IP address: %s\n", group);
            continue;
        }
        int is_general_query = (strcmp(group, "0.0.0.0") == 0);
        const char *dst_ip = is_general_query ? "224.0.0.1" : group;

        printf("[TEST] Sending IGMPv2 %s Query to %s (group field: %s) via %s\n",
               is_general_query ? "General" : "Group-Specific", dst_ip, group, INTERFACE);

        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
        if (sock < 0) {
            perror("socket");
            free(groups);
            return 1;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE, strlen(INTERFACE)) < 0) {
            perror("SO_BINDTODEVICE");
            close(sock);
            free(groups);
            return 1;
        }

        uint8_t ttl = 1;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
            perror("setsockopt: IP_MULTICAST_TTL");
            close(sock);
            free(groups);
            return 1;
        }

        struct igmp msg = {0};
        msg.igmp_type = 0x11;
        msg.igmp_code = 20;
        inet_pton(AF_INET, group, &msg.igmp_group);
        msg.igmp_cksum = checksum(&msg, sizeof(msg));

        struct sockaddr_in dst = {0};
        dst.sin_family = AF_INET;
        inet_pton(AF_INET, dst_ip, &dst.sin_addr);

        ssize_t sent = sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&dst, sizeof(dst));
        if (sent < 0) {
            perror("sendto");
        } else {
            printf("[TEST] IGMPv2 Query sent to %s (group field: %s)\n", dst_ip, group);
        }

        close(sock);
        sleep(QUERY_INTERVAL_SEC);
    }
}