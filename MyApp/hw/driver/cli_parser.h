#ifndef SRC_HW_DRIVER_CLI_PARSER_H_
#define SRC_HW_DRIVER_CLI_PARSER_H_

#include "cli_priv.h"

void cliParserInit(void);
bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char **argv));
void cliParseArgs(char *line_buf);
void cliRunCommand(void);

#endif /* SRC_HW_DRIVER_CLI_PARSER_H_ */
