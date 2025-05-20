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

void handle_event(GroupInfo *group, GroupEvent event);

#endif //FSM_H
