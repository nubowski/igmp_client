#include <stdio.h>

#include "fsm.h"

void handle_event(GroupInfo *group, GroupEvent event) {
    printf("[FSM] Event %d on group %s (state %d\n", event, group->group_ip, group->state);
}
