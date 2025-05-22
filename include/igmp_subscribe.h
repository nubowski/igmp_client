#ifndef IGMP_SUBSCRIBE_H
#define IGMP_SUBSCRIBE_H

int init_igmp_socket(const char *iface);
int igmp_subscribe(const char *group_ip, const char *iface);
int igmp_unsubscribe(const char *group_ip, const char *iface);
int get_igmp_socket(void);

#endif //IGMP_SUBSCRIBE_H
