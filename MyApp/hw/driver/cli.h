#ifndef SRC_HW_DRIVER_CLI_H_
#define SRC_HW_DRIVER_CLI_H_

#include "hw_def.h"

void cliInit(void);
void cliMain(void);
void cliPrintf(char *fmt, ...);
bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char **argv));

void cliParseArgs(char *line_buf);
void cliRunCommand(void);

#endif
