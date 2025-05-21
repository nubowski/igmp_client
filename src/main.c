#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "igmp.h"
#include "cli.h"
#include "fsm.h"
#include "utils.h"

// CLI help
void print_usage(const char *prog) {
    printf("Usage: %s -i <interface> -g <group> [-g <group2> ...] -t timer\n", prog);
    printf("  --igmpv1, --v1      Enable IGMPv1 suppression mode\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));      // RFC: rnd timer (need to take a seed at start)

    static struct option long_options[] = {
        {"igmpv1", no_argument, NULL, 1000},   // RFC: IGMPv1 suppression  "SHOULD skip.."
        {"v1",     no_argument, NULL, 1000},   // alias for --igmpv1
        {0, 0, 0, 0}
    };

    ClientConfig config = {0};

    int opt;
    while ((opt = getopt_long(argc, argv, "i:g:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i':
                // our working IGMP iface
                strncpy(config.interface, optarg, sizeof(config.interface) - 1);
                break;
            case 'g':
                // args count (`optarg`) + how many after `-g`
                int group_argc = 1;
                while (optind < argc && argv[optind][0] != '-') {
                    group_argc++;
                    optind++;
                }

                // back to start
                optind -= group_argc - 1;

                for (int i = 0; i < group_argc; i++) {
                    const char *ip = (i == 0) ? optarg : argv[optind++];
                    if (config.group_count >= MAX_GROUPS) {
                        fprintf(stderr, "Too many groups (max %d)\n", MAX_GROUPS);
                        exit(1);
                    }
                    if (!is_valid_ipv4(ip)) {
                        fprintf(stderr, "Invalid IP address: %s\n", ip);
                        exit(1);
                    }
                    strncpy(config.groups[config.group_count], ip, sizeof(config.groups[0]) - 1);
                    config.group_count++;
                }
                break;
            case 't':
                // add Max Resp Time
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

    // Base validation
    if (config.interface[0] == '\0' || config.group_count == 0) {
        print_usage(argv[0]);
        return 1;
    }


    // Bootstrap
    print_status_info(&config, get_max_response_time(), is_igmpv1_enabled());

    fsm_set_iface(config.interface);
    send_igmp_reports(&config);
    start_fsm_timer_loop();
    start_cli_loop();               // important to start cli stdin thread before listener
    start_igmp_listener(&config);
    return 0;
}
