#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cli.h"
#include "fsm.h"
#include "igmp.h"

void print_usage(const char *prog) {
    printf("Usage: %s -i <interface> -g <group> [-g <group2> ...]\n", prog);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    ClientConfig config = {0};

    int opt;
    while ((opt = getopt(argc, argv, "i:g:t:")) != -1) {
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
            case 't':
                char *endptr;
                long val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || val <= 0 || val > 255000) {
                    fprintf(stderr, "Invalid max response time. Should be [1-255000]. Falling to default 2000.\n");
                } else {
                    fsm_set_max_response_time((int)val);
                }
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

    fsm_set_iface(config.interface);

    send_igmp_reports(&config);
    start_fsm_timer_loop();
    start_cli_loop();               // important to start clie stdin thread before listener
    start_igmp_listener(&config);
    return 0;
}
