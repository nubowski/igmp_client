#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

static int igmp_socket = -1;

int init_igmp_socket(const char *iface) {
    igmp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    if (igmp_socket < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(igmp_socket, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface)) < 0) {
        perror("SO_BINDTODEVICE");
        close(igmp_socket);
        return -1;
    }

    return 0;
}

int igmp_subscribe(const char *group_ip, const char *iface) {
    struct ip_mreqn mreq = {0};
    inet_pton(AF_INET, group_ip, &mreq.imr_multiaddr);
    mreq.imr_address.s_addr = htonl(INADDR_ANY);
    mreq.imr_ifindex = if_nametoindex(iface);
    return setsockopt(igmp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}

int igmp_unsubscribe(const char *group_ip, const char *iface) {
    struct ip_mreqn mreq = {0};
    inet_pton(AF_INET, group_ip, &mreq.imr_multiaddr);
    mreq.imr_address.s_addr = htonl(INADDR_ANY);
    mreq.imr_ifindex = if_nametoindex(iface);
    return setsockopt(igmp_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
}

int get_igmp_socket(void) {
    return igmp_socket;
}

