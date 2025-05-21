#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "igmp.h"
#include "fsm.h"
#include "cli.h"
#include "utils.h"

// func ptr
typedef void (*CliCommandHandler)(const char *args);

// entry cmd type
typedef struct {
    const char *name;
    const char *desc;
    CliCommandHandler handler;
} CliCommand;

static void cmd_add(const char *args);
static void cmd_del(const char *args);
static void cmd_print(const char *args);
static void cmd_help(const char *args);
static void cmd_exit(const char *args);
static void cmd_set_resp(const char *args);
static void cmd_status(const char *args);

static CliCommand commands[] = {
    { "add",        "Join multicast group (add <ip>)",                  cmd_add },
    { "del",        "Leave multicast group (del <ip>)",                 cmd_del },
    { "print",      "Print all known groups",                           cmd_print },
    { "help",       "Show this help message",                           cmd_help },
    { "exit",       "Exit the application",                             cmd_exit },
    { "set_timer",  "Set max response time in ms (set_timer <ms>)",     cmd_set_resp },
    { "status",     "Show current status",                              cmd_status },
};

static const int command_count = sizeof(commands) / sizeof(commands[0]);

// --- Command handlers --- //

static void cmd_add(const char *args) {
    if (!args) {
        printf("Usage: add <group_ip>\n");
        return;
    }

    if (!is_valid_ipv4(args)) {
        printf("Invalid IP address format: %s\n", args);
        return;
    }

    GroupInfo *g = find_or_create_group(args);
    if (g) {
        handle_event(g, EV_JOIN_GROUP);
    }
}

static void cmd_del(const char *args) {
    if (!args) {
        printf("Usage: del <group_ip>\n");
        return;
    }

    GroupInfo *g = find_group(args);
    if (!g) {
        printf("Group not found: %s\n", args);
        return;
    }

    handle_event(g, EV_LEAVE_GROUP);

    if (remove_group(args)) {
        printf("Group %s removed from list.\n", args);
    } else {
        printf("Failed to remove group %s\n", args);
    }
}

static void cmd_print(const char *args) {
    (void)args;
    print_all_groups();
}

static void cmd_help(const char *args) {
    (void)args;

    printf(ANSI_CYAN "\n\n=== IGMP Client Command Help ===\n\n" ANSI_RESET);

    int max_len = 0;
    for (int i = 0; i < command_count; i++) {
        int len = (int)strlen(commands[i].name);
        if (len > max_len) max_len = len;
    }

    printf(ANSI_BOLD "  %-*s  - %s\n", max_len, "Command", "Description" ANSI_RESET);
    printf(ANSI_BOLD "  %-*s  - %s\n", max_len, "-------", "-----------" ANSI_RESET);

    for (int i = 0; i < command_count; i++) {
        printf("  %-*s  - %s\n", max_len, commands[i].name, commands[i].desc);
    }

    printf(ANSI_YELLOW "\nType a command and hit Enter. Example: `add 239.1.1.1`\n" ANSI_RESET);
    printf("Use `exit` to quit.\n\n");
}

static void cmd_exit(const char *args) {
    (void)args;
    printf(ANSI_GREEN "Shutting down...\n" ANSI_RESET);
    exit(0);
}

static void cmd_set_resp(const char *args) {
    if (!args) {
        printf(ANSI_RED "Usage: set_resp <ms> [1-255000]\n" ANSI_RESET);
        return;
    }

    int val = 0;
    if (!parse_int(args, &val) || val < 1 || val > 255000) {
        printf(ANSI_RED "Invalid response time. Must be integer in range [1-255000]\n" ANSI_RESET);
        return;
    }

    fsm_set_max_response_time(val);
    printf(ANSI_GREEN "Response time set to %dms\n" ANSI_RESET, val);
}

static void cmd_status(const char *args) {
    (void)args;
    ClientConfig cfg = {0};

    strncpy(cfg.interface, get_iface_name(), sizeof(cfg.interface) - 1);

    int count = get_group_count();
    cfg.group_count = count;

    for (int i = 0; i < count; i++) {
        const char *ip = get_group_ip_at(i);
        if (ip) {
            strncpy(cfg.groups[i], ip, sizeof(cfg.groups[i]) - 1);
        }
    }

    print_startup_info(&cfg, get_max_response_time(), is_igmpv1_enabled());
}

// --- CLI loop thread --- //

static void *cli_thread(void *arg) {
    (void)arg;
    char input[128];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';          // remove nl

        char *space = strchr(input, ' ');
        char *cmd = input;
        char *args = NULL;

        if (space) {
            *space = '\0';
            args = space + 1;
        }

        int matched = 0;
        for (int i = 0; i < command_count; i++) {
            if (strcmp(cmd, commands[i].name) == 0) {
                commands[i].handler(args);
                matched = 1;
                break;
            }
        }

        if (matched == 0 && strlen(cmd) > 0) {
            printf("Unknown command: '%s' (try `help`)\n", cmd);
        }
    }

    return NULL;
}

void start_cli_loop(void) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, cli_thread, NULL)) {
        perror("pthread_create (CLI)");
        exit(1);
    }
}

void print_startup_info(const ClientConfig *cfg, int max_resp_time, int is_v1_enabled) {
    printf(ANSI_BOLD ANSI_CYAN "\n╔═══════════════════════════════════════════════╗\n");
    printf("║               IGMPv2 Client Started           ║\n");
    printf("╠═══════════════════════════════════════════════╣\n" ANSI_RESET);

    printf(ANSI_YELLOW " Interface       " ANSI_RESET ": %s\n", cfg->interface);

    printf(ANSI_YELLOW " Multicast groups" ANSI_RESET ":\n");
    for (int i = 0; i < cfg->group_count; i++) {
        printf("   " ANSI_GREEN "•" ANSI_RESET " %s\n", cfg->groups[i]);
    }

    printf(ANSI_YELLOW " Max Resp Time   " ANSI_RESET ": %d ms\n", max_resp_time);
    printf(ANSI_YELLOW " IGMPv1 Suppress " ANSI_RESET ": %s\n",
           is_v1_enabled ? ANSI_GREEN "ENABLED" ANSI_RESET : ANSI_MAGENTA "DISABLED" ANSI_RESET);

    printf(ANSI_BOLD ANSI_CYAN "╚═══════════════════════════════════════════════╝\n\n" ANSI_RESET);
}

void print_all_groups(void) {
    int group_count = get_group_count();

    printf(ANSI_BOLD ANSI_CYAN "\n╔═══════════════════════════════════════════════╗\n");
    printf("║              Current Group States             ║\n");
    printf("╠═══════════════════════════════════════════════╣\n" ANSI_RESET);

    printf(ANSI_YELLOW " Interface" ANSI_RESET ": %s\n", get_iface_name());
    printf(ANSI_YELLOW " Group Count" ANSI_RESET ": %d\n\n", group_count);

    if (group_count == 0) {
        printf(ANSI_RED " No active groups.\n" ANSI_RESET);
        printf(ANSI_BOLD ANSI_CYAN "╚═══════════════════════════════════════════════╝\n\n" ANSI_RESET);
        return;
    }

    printf(ANSI_BOLD " %-12s  %-16s  %-8s   %-4s\n" ANSI_RESET,
           "Group", "State", "Timer", "LR");

    printf(ANSI_BOLD " %-12s  %-16s  %-8s   %-4s\n" ANSI_RESET,
           "------", "-----", "-----", "--");

    for (int i = 0; i < group_count; i++) {
        GroupInfo *g = get_group_at(i);
        const char *state_str = state_to_str(g->state);
        const char *reporter_str = g->last_reporter
            ? ANSI_GREEN "Yes" ANSI_RESET
            : ANSI_RED   "No"  ANSI_RESET;

        printf(" %-12s  %-16s  %-4dms     %-4s\n",
               g->group_ip,
               state_str,
               g->timer_ms,
               reporter_str);
    }

    printf(ANSI_BOLD ANSI_CYAN "\n╚═══════════════════════════════════════════════╝\n\n" ANSI_RESET);
}