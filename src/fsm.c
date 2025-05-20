#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "fsm.h"

#define MAX_KNOWN_GROUPS 64

static GroupInfo group_states[MAX_KNOWN_GROUPS];
static int group_count = 0;

static void action_join(GroupInfo *group) {
    printf("[FSM] Joining group %s\n", group->group_ip);
    group->timer_ms = 1000;                                     // TODO: make rnd timer, to prevent reply-bursts
}

static void action_send_report(GroupInfo *group) {
    printf("[FSM] Sending report to group %s\n", group->group_ip);
}

static void action_leave(GroupInfo *group) {
    printf("[FSM] Leaving group %s\n", group->group_ip);
}

static void action_nop(GroupInfo *group) {
    (void) group;
    // do nothing
}

// FSM mapping state. Creating some beautiful ru4noi kolhoz
static const FsmEntry fsm_map[3][4] = {
    [NON_MEMBER] = {
        [EV_JOIN_GROUP]     = { DELAYING_MEMBER,    action_join },
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_nop },
        [EV_QUERY_RECEIVED] = { NON_MEMBER,         action_nop },
        [EV_TIMER_EXPIRED]  = { NON_MEMBER,         action_nop },
    },
    [DELAYING_MEMBER] = {
        [EV_JOIN_GROUP]     = { DELAYING_MEMBER,    action_join },      // drop timer
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_leave },
        [EV_QUERY_RECEIVED] = { DELAYING_MEMBER,    action_nop },       // TODO: timer refresh??
        [EV_TIMER_EXPIRED]  = { NON_MEMBER,         action_send_report },
    },
    [IDLE_MEMBER] = {
        [EV_JOIN_GROUP]     = { IDLE_MEMBER,        action_nop },
        [EV_LEAVE_GROUP]    = { NON_MEMBER,         action_leave },
        [EV_QUERY_RECEIVED] = { DELAYING_MEMBER,    action_join },
        [EV_TIMER_EXPIRED]  = { IDLE_MEMBER,        action_nop },
    }
};

void handle_event(GroupInfo *group, GroupEvent event) {
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
    group_states[group_count].state = NON_MEMBER;
    group_states[group_count].timer_ms = 0;
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
        printf(" - %s (state=%d, timer=%dms)\n", group_states[i].group_ip,
               group_states[i].state, group_states[i].timer_ms);
    }
}

// TODO: need some customizable cozy things
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

