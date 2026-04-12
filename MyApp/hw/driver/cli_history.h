#ifndef SRC_HW_DRIVER_CLI_HISTORY_H_
#define SRC_HW_DRIVER_CLI_HISTORY_H_

#include "cli_priv.h"

void cliHistoryInit(void);
void cliHistoryPush(const char* cmd_str);
bool cliHistoryGetPrev(char* out_buf);
bool cliHistoryGetNext(char* out_buf);
void cliHistoryResetDepth(void);

#endif /* SRC_HW_DRIVER_CLI_HISTORY_H_ */
