#include "cli_history.h"
#include <string.h>

static char    hist_buf[CLI_HIST_MAX][CLI_LINE_BUF_MAX];
static uint8_t hist_count = 0;
static uint8_t hist_write = 0;
static uint8_t hist_depth = 0;

void cliHistoryInit(void)
{
    hist_count = 0;
    hist_write = 0;
    hist_depth = 0;
    for (int i = 0; i < CLI_HIST_MAX; i++) {
        hist_buf[i][0] = '\0';
    }
}

void cliHistoryPush(const char* cmd_str)
{
    strncpy(hist_buf[hist_write], cmd_str, CLI_LINE_BUF_MAX - 1);
    hist_buf[hist_write][CLI_LINE_BUF_MAX - 1] = '\0';
    
    hist_write = (hist_write + 1) % CLI_HIST_MAX;
    if (hist_count < CLI_HIST_MAX) {
        hist_count++;
    }
    hist_depth = 0; // 엔터를 치면 탐색 깊이 반환
}

bool cliHistoryGetPrev(char* out_buf)
{
    if (hist_depth < hist_count) {
        hist_depth++;
        int idx = (hist_write - hist_depth + CLI_HIST_MAX) % CLI_HIST_MAX;
        strncpy(out_buf, hist_buf[idx], CLI_LINE_BUF_MAX - 1);
        out_buf[CLI_LINE_BUF_MAX - 1] = '\0';
        return true;
    }
    return false;
}

bool cliHistoryGetNext(char* out_buf)
{
    if (hist_depth > 1) {
        hist_depth--;
        int idx = (hist_write - hist_depth + CLI_HIST_MAX) % CLI_HIST_MAX;
        strncpy(out_buf, hist_buf[idx], CLI_LINE_BUF_MAX - 1);
        out_buf[CLI_LINE_BUF_MAX - 1] = '\0';
        return true;
    } 
    else if (hist_depth == 1) {
        hist_depth--;
        out_buf[0] = '\0'; // 가장 최근(미래)로 오면 빈 칸
        return true;
    }
    return false;
}

void cliHistoryResetDepth(void)
{
    hist_depth = 0;
}
