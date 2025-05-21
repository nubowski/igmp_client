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
    EV_TIMER_EXPIRED,
    EV_REPORT_RECEIVED
} GroupEvent;

typedef struct {
    char group_ip[16];
    GroupState state;
    int timer_ms;
    int last_reporter;
} GroupInfo;

typedef void (*FsmAction)(GroupInfo*);

typedef struct {
    GroupState next_state;
    FsmAction action;
} FsmEntry;

void handle_event(GroupInfo *group, GroupEvent event);

GroupInfo *find_or_create_group(const char *group_ip);
GroupInfo *find_group(const char *group_ip);
int get_group_count(void);
GroupInfo *get_group_at(int index);
void print_all_groups(void);

void start_fsm_timer_loop(void);
void fsm_set_max_response_time(int ms);

void fsm_set_iface(const char *iface);
void fsm_set_igmpv1_mode(int enabled);

#endif //FSM_H
