#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "fsm.h"
#include "igmp.h"
#include "utils.h"

#define MAX_KNOWN_GROUPS 64

static int max_resp_time_ms = 2000;                 // max value 1byte ~ 25500ms, but RFC: 1000-2000ms
static int support_igmpv1 = 0;                      // 0 = v1 mode, suppress Leave

// TODO: need to figure out better access for that
static char current_iface[32] = "eth0";             // default interface

static GroupInfo group_states[MAX_KNOWN_GROUPS];
static int group_count = 0;

// RFC: Join behaves as if Group-Specific Query received → start timer
static void action_join(GroupInfo *group) {
    group->timer_ms = random_uniform(max_resp_time_ms);
    printf("[FSM] Joining group %s\n", group->group_ip);
}

static void action_send_report(GroupInfo *group) {
    printf("[FSM] Sending report to group %s\n", group->group_ip);
    send_igmp_report(group->group_ip, current_iface);
    group->last_reporter = 1;  // RFC: set flag that we were the last host
}

static void action_leave(GroupInfo *group) {
    // RFC: If the interface state says the Querier is running IGMPv1, this action SHOULD be skipped
    // RFC: If the flag saying we were the last host to report is cleared, this action MAY be skipped
    if (support_igmpv1 == 1) {
        printf("[FSM] IGMPv1 mode → suppressing Leave\n");
        return;
    }

    if (group->last_reporter) {
        send_igmp_leave(group->group_ip, current_iface);
    } else {
        printf("[FSM] Not last reporter — skipping Leave\n");
    }
}

static void action_reset_timer(GroupInfo *group) {
    int delay = random_uniform(max_resp_time_ms);
    if (delay < group->timer_ms) {
        group->timer_ms = delay;
        printf("[FSM] Timer reset for group %s to %dms\n", group->group_ip, delay);
    } else {
        printf("[FSM] Timer NOT reset for group %s (delay=%d >= current=%d)\n",
               group->group_ip, delay, group->timer_ms);
    }
}

static void action_stop_timer(GroupInfo *group) {
    // No RFC directions or suggestion of holding the timer value nor its reset to 0. So timer_ms = 0 working the way we need
    group->timer_ms = 0;
    group->last_reporter = 0;  // RFC: clear flag
    printf("[FSM] Report received for %s → timer stopped (suppression)\n", group->group_ip);
}

static void action_nop(GroupInfo *group) {
    (void) group;
    // do nothing
}

// FSM mapping state. Creating some beautiful ru4noi kolhoz
// RFC: Host State Diagram (FSM mapping)
static const FsmEntry fsm_map[3][5] = {
    [NON_MEMBER] = {
        [EV_JOIN_GROUP]     = { DELAYING_MEMBER,    action_join },
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_nop },
        [EV_QUERY_RECEIVED] = { NON_MEMBER,         action_nop },
        [EV_TIMER_EXPIRED]  = { NON_MEMBER,         action_nop },
        [EV_REPORT_RECEIVED]= { NON_MEMBER,         action_nop },
    },
    [DELAYING_MEMBER] = {
        [EV_JOIN_GROUP]     = { DELAYING_MEMBER,    action_join },
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_leave },
        [EV_QUERY_RECEIVED] = { DELAYING_MEMBER,    action_reset_timer },
        [EV_TIMER_EXPIRED]  = { NON_MEMBER,         action_send_report },
        [EV_REPORT_RECEIVED]= { IDLE_MEMBER,        action_stop_timer },
    },
    [IDLE_MEMBER] = {
        [EV_JOIN_GROUP]     = { IDLE_MEMBER,        action_nop },
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_leave },
        [EV_QUERY_RECEIVED] = { DELAYING_MEMBER,    action_join },
        [EV_TIMER_EXPIRED]  = { IDLE_MEMBER,        action_nop },
        [EV_REPORT_RECEIVED]= { IDLE_MEMBER,        action_nop },
    }
};

void handle_event(GroupInfo *group, GroupEvent event) {
    if (strcmp(group->group_ip, "224.0.0.1") == 0) {
        // RFC: do nothing
        return;
    }

    printf("[FSM] Event %d on group %s (state %d)\n", event, group->group_ip, group->state);
    FsmEntry entry = fsm_map[group->state][event];
    group->state = entry.next_state;
    if (entry.action) {
        entry.action(group);
    }
}

GroupInfo *find_or_create_group(const char *group_ip) {
    for (int i = 0; i < group_count; i++) {
        if (strcmp(group_states[i].group_ip, group_ip) == 0) {
            return &group_states[i];
        }
    }

    if (group_count >= MAX_KNOWN_GROUPS) {
        fprintf(stderr, "[FSM] Too many group states\n");
        return NULL;
    }

    // creating then
    strncpy(group_states[group_count].group_ip, group_ip, sizeof(group_states[group_count].group_ip) - 1);

    if (strcmp(group_ip, "224.0.0.1") == 0) {
        group_states[group_count].state = IDLE_MEMBER;
        group_states[group_count].timer_ms = 0;
        printf("[FSM] Initialized 224.0.0.1 as special Idle Member (static)\n");
    } else {
        group_states[group_count].state = NON_MEMBER;
        group_states[group_count].timer_ms = 0;
    }

    return &group_states[group_count++];
}

GroupInfo *find_group(const char *group_ip) {
    for (int i = 0; i < group_count; i++) {
        if (strcmp(group_states[i].group_ip, group_ip) == 0) {
            return &group_states[i];
        }
    }
    return NULL;
}

int get_group_count(void) {
    return group_count;
}

GroupInfo *get_group_at(int index) {
    if (index >= 0 && index < group_count) {
        return &group_states[index];
    }
    return NULL;
}

void print_all_groups(void) {
    printf("[FSM] Current group states:\n");
    for (int i = 0; i < group_count; i++) {
        printf(" - %s (state=%d, timer=%dms, last_reporter=%d)\n",
               group_states[i].group_ip,
               group_states[i].state,
               group_states[i].timer_ms,
               group_states[i].last_reporter);
    }
}

void fsm_set_max_response_time(int ms) {
    max_resp_time_ms = ms;
}

// RFC: Max Response Time is expressed in 1/10 seconds
void *fsm_timer_thread(void *arg) {
    (void)arg;

    while (1) {
        usleep(100 * 1000);                 // 100 ms

        for (int i = 0; i < group_count; i++) {
            GroupInfo *g = &group_states[i];
            if (g->timer_ms > 0) {
                g->timer_ms -= 100;
                if (g->timer_ms <= 0) {
                    handle_event(g, EV_TIMER_EXPIRED);
                }
            }
        }
    }

    return NULL;
}

void start_fsm_timer_loop(void) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, fsm_timer_thread, NULL)) {
        perror("pthread_create");
        exit(1);
    }
}

void fsm_set_iface(const char *iface) {
    strncpy(current_iface, iface, sizeof(current_iface) - 1);
}

void fsm_set_igmpv1_mode(int enabled) {
    support_igmpv1 = enabled ? 1 : 0;
}

int get_max_response_time(void) {
    return max_resp_time_ms;
}

int is_igmpv1_enabled(void) {
    return support_igmpv1;
}

