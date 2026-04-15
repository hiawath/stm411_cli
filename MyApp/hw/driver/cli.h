#ifndef SRC_HW_DRIVER_CLI_H_
#define SRC_HW_DRIVER_CLI_H_

#include "hw_def.h"
#include "cli_parser.h"

typedef void (*cli_callback_t)(void);

void cliInit(void);
void cliMain(void);
void cliPrintf(char *fmt, ...);
void cliSetCtrlCHandler(cli_callback_t handler);

#endif
