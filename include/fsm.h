#ifndef FSM_H
#define FSM_H

typedef enum {
    NON_MEMBER,
    DELAYING_MEMBER,
    IDLE_MEMBER
} GroupState;

typedef enum {
    EV_JOIN_GROUP,
    EV_LEAVE_GROUP,
    EV_QUERY_RECEIVED,
    EV_TIMER_EXPIRED
} GroupEvent;

typedef struct {
    char group_ip[16];
    GroupState state;
    int timer_ms;
} GroupInfo;

typedef void (*FsmAction)(GroupInfo*);

typedef struct {
    GroupState next_state;
    FsmAction action;
} FsmEntry;

void handle_event(GroupInfo *group, GroupEvent event);
GroupInfo *find_or_create_group(const char *group_ip);
GroupInfo *find_group(const char *group_ip);
void print_all_groups(void);

void start_fsm_timer_loop(void);

#endif //FSM_H
