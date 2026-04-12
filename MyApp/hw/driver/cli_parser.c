#include "cli_parser.h"
#include "cli.h" // cliPrintf 등을 사용하기 위해 포함
#include <string.h>

typedef struct {
    char cmd_str[16];
    void (*cmd_func)(uint8_t argc, char **argv);
} cli_cmd_t;

static cli_cmd_t cli_cmd_list[CLI_CMD_LIST_MAX]; 
static uint8_t   cli_cmd_count = 0;

static uint8_t   cli_argc = 0; 
static char*     cli_argv[CLI_CMD_ARG_MAX]; 

static void cliHelp(uint8_t argc, char **argv);
static void cliClear(uint8_t argc, char **argv);

void cliParserInit(void)
{
    cli_cmd_count = 0;
    cliAdd("help", cliHelp);
    cliAdd("cls", cliClear);
}

void cliParseArgs(char *line_buf)
{
    char *tok;
    
    cli_argc = 0; 
    tok = strtok(line_buf, " ");
    
    while(tok != NULL && cli_argc < CLI_CMD_ARG_MAX)
    {
        cli_argv[cli_argc] = tok; 
        cli_argc++;
        tok = strtok(NULL, " ");  
    }
}

void cliRunCommand(void)
{
    if (cli_argc == 0) return; 
    
    bool is_found = false;

    for (int i = 0; i < cli_cmd_count; i++)
    {
        if (strcmp(cli_argv[0], cli_cmd_list[i].cmd_str) == 0)
        {
            is_found = true;
            cli_cmd_list[i].cmd_func(cli_argc, cli_argv);
            break;
        }
    }
    
    if (is_found == false) 
    {
        cliPrintf("Command Not Found\r\n");
    }
}

bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char **argv))
{
    if (cli_cmd_count >= CLI_CMD_LIST_MAX)
    {
        return false; 
    }

    strncpy(cli_cmd_list[cli_cmd_count].cmd_str, cmd_str, sizeof(cli_cmd_list[0].cmd_str) - 1);
    cli_cmd_list[cli_cmd_count].cmd_func = cmd_func;
    cli_cmd_count++;
    
    return true;
}

static void cliHelp(uint8_t argc, char **argv)
{
    cliPrintf("--------------- CLI Commands ---------------\r\n");
    for (int i = 0; i < cli_cmd_count; i++)
    {
        cliPrintf("  %s\r\n", cli_cmd_list[i].cmd_str);
    }
    cliPrintf("--------------------------------------------\r\n");
}

static void cliClear(uint8_t argc, char **argv)
{
    cliPrintf("\x1B[2J\x1B[H");
}
