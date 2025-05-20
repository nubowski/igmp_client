#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "igmp.h"

void print_usage(const char *prog) {
    printf("Usage: %s -i <interface> -g <group> [-g <group2> ...]\n", prog);
}

int main(int argc, char *argv[]) {
    ClientConfig config = {0};

    int opt;
    while ((opt = getopt(argc, argv, "i:g:")) != -1) {
        switch (opt) {
            case 'i':
                strncpy(config.interface, optarg, sizeof(config.interface) - 1);
                break;
            case 'g':
                if (config.group_count >= MAX_GROUPS) {
                    fprintf(stderr, "Too many groups (max %d)\n", MAX_GROUPS);
                    exit(1);
                }
                int index = config.group_count;
                strncpy(config.groups[index], optarg, sizeof(config.groups[index]) - 1);
                config.group_count++;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (config.interface[0] == '\0' || config.group_count == 0) {
        print_usage(argv[0]);
        return 1;
    }

    print_config(&config);
    send_igmp_reports(&config);
    return 0;
}
