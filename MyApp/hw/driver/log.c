#include "log.h"
#include "cli_parser.h"
#include "log_def.h"
#include "cli.h"
#include "uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t runtime_log_level = 3; // 기본값: INFO (3)

// CLI 명령어 콜백 함수
void cliLog(uint8_t argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "get") == 0)
    {
        cliPrintf("Current Log Level: %d\r\n", runtime_log_level);
    }
    else if (argc == 3 && strcmp(argv[1], "set") == 0)
    {
        uint8_t level = atoi(argv[2]);
        if (level <= 5)
        {
            logSetLevel(level);
            cliPrintf("Log Level Set to %d\r\n", level);
        }
        else
        {
            cliPrintf("Invalid Level (0~5)\r\n");
        }
    }
    else
    {
        cliPrintf("0:FATAL,1:ERROR,2:WARN,3:INFO,4:DEBUG,5:VERBOSE\r\n");
        cliPrintf("Usage: log get\r\n");
        cliPrintf("       log set [0~5]\r\n");
    }
}

bool logInit(void)
{
    cliAdd("log", cliLog);
    return true;
}

void logSetLevel(uint8_t level)
{
    runtime_log_level = level;
}

uint8_t logGetLevel(void)
{
    return runtime_log_level;
}

// log.h에서 사용하는 런타임 레벨 획득 함수 (외부 노출용)
uint8_t logGetRuntimeLevel(void)
{
    return runtime_log_level;
}

// 로그 포맷 출력을 위한 전용 printf (uartWrite 호출)
void logPrintf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    // UART 채널 0으로 출력 (uartWrite 내부에서 Mutex 보호됨)
    uartWrite(0, (uint8_t *)buf, len);
}
