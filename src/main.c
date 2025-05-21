#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "cli.h"
#include "fsm.h"
#include "igmp.h"
#include "utils.h"

void print_usage(const char *prog) {
    printf("Usage: %s -i <interface> -g <group> [-g <group2> ...] -t timer\n", prog);
    printf("  --igmpv1, --v1      Enable IGMPv1 suppression mode\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    static struct option long_options[] = {
        {"igmpv1", no_argument, NULL, 1000},   // unique int > 255
        {"v1",     no_argument, NULL, 1000},   // alias for --igmpv1
        {0, 0, 0, 0}
    };

    ClientConfig config = {0};

    int opt;
    while ((opt = getopt_long(argc, argv, "i:g:t:", long_options, NULL)) != -1) {
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
                int val = 0;
                if (!parse_int(optarg, &val) || val <= 0 || val > 255000) {
                    fprintf(stderr, "Invalid max response time. Should be [1-255000]. Falling to default 2000.\n");
                } else {
                    fsm_set_max_response_time(val);
                }
                break;
            case 1000:
                fsm_set_igmpv1_mode(1);
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
