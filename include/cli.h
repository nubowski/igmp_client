#ifndef CLI_H
#define CLI_H

void start_cli_loop(void);
void print_startup_info(const ClientConfig *cfg, int max_resp_time, int is_v1_enabled);

#endif //CLI_H
