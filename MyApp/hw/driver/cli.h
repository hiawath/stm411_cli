#ifndef SRC_HW_DRIVER_CLI_H_
#define SRC_HW_DRIVER_CLI_H_

#include "hw_def.h"
#include "cli_parser.h"

void cliInit(void);
void cliMain(void);
void cliPrintf(char *fmt, ...);

#endif
