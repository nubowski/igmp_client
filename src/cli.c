#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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

static CliCommand commands[] = {
    { "add",        "Join multicast group (add <ip>)",                  cmd_add },
    { "del",        "Leave multicast group (del <ip>)",                 cmd_del },
    { "print",      "Print all known groups",                           cmd_print },
    { "help",       "Show this help message",                           cmd_help },
    { "exit",       "Exit the application",                             cmd_exit },
    { "set_timer",  "Set max response time in ms (set_timer <ms>)",     cmd_set_resp },
};

static const int command_count = sizeof(commands) / sizeof(commands[0]);

// --- Command handlers --- //

static void cmd_add(const char *args) {
    if (!args) {
        printf("Usage: add <group_ip>\n");
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
    if (g) {
        handle_event(g, EV_LEAVE_GROUP);
    }
}

static void cmd_print(const char *args) {
    (void)args;
    print_all_groups();
}

static void cmd_help(const char *args) {
    (void)args;
    printf("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        printf(" - %s: %s\n", commands[i].name, commands[i].desc);
    }
}

static void cmd_exit(const char *args) {
    (void)args;
    printf("Shutting down...\n");
    exit(0);
}

static void cmd_set_resp(const char *args) {
    if (!args) {
        printf("Usage: set_resp <ms> [1-255000]\n");
        return;
    }

    int val = 0;
    if (!parse_int(args, &val) || val < 1 || val > 255000) {
        printf("Invalid response time. Must be integer in range [1-255000]\n");
        return;
    }

    fsm_set_max_response_time(val);
    printf("Response time set to %dms\n", val);
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

void print_config(const ClientConfig *cfg) {
    printf("Interface: %s\n", cfg->interface);
    printf("Groups:\n");

    for (int i = 0; i < cfg->group_count; i++) {
        printf(" - %s\n", cfg->groups[i]);
    }
}