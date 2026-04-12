// MyApp/hw/driver/cli.c
#include "cli.h"
#include "cli_priv.h"  
#include "cli_history.h" 
#include "cli_parser.h"  // 파서 모듈 추가
#include "uart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// ============================================
// 정적(Static) 변수 할당 공간
// ============================================

static char      cli_line_buf[CLI_LINE_BUF_MAX]; 
static uint16_t  cli_line_idx = 0; // 총 담긴 글자 수 저장
static uint16_t  cli_cursor = 0;   // 현재 깜빡이는 논리적 커서 위치

typedef enum {
    CLI_STATE_NORMAL = 0,
    CLI_STATE_ESC_RCVD,
    CLI_STATE_BRACKET_RCVD
} cli_input_state_t;

static cli_input_state_t input_state = CLI_STATE_NORMAL;

void cliPrintf(char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    uartWrite(0, (uint8_t *)buf, len); // 무조건 터미널이 연결된 채널(0)으로 쏨
}

// === 리팩토링된 CLI 핸들러 함수들 ===

static void cliRedrawTail(void) 
{
    for (int i = cli_cursor; i < cli_line_idx; i++) {
        cliPrintf("%c", cli_line_buf[i]);
    }
    cliPrintf(" \b"); // 흔적 지우기
    
    for (int i = 0; i < (cli_line_idx - cli_cursor); i++) {
        cliPrintf("\b");
    }
}

static void handleEnterKey(void)
{
    // 커서를 맨 끝으로 보냄
    for (int i = cli_cursor; i < cli_line_idx; i++) cliPrintf("\x1B[C");
    cliPrintf("\r\n"); 
    
    if (cli_line_idx > 0) 
    {
        cli_line_buf[cli_line_idx] = '\0';
        
        cliHistoryPush(cli_line_buf); // 과거 기록 버퍼에 밀어넣기
        cliParseArgs(cli_line_buf); 
        cliRunCommand();
    }
    
    cliPrintf("CLI> ");
    cli_line_idx = 0;
    cli_cursor = 0;
}

static void handleBackspace(void)
{
    if (cli_cursor == 0) return;
    
    for (int i = cli_cursor; i < cli_line_idx; i++) {
        cli_line_buf[i - 1] = cli_line_buf[i];
    }
    cli_line_idx--;
    cli_cursor--;
    
    cliPrintf("\b"); 
    cliRedrawTail();
}

static void handleCharInsert(uint8_t c)
{
    if (cli_line_idx >= CLI_LINE_BUF_MAX - 1) return;
    
    for (int i = cli_line_idx; i > cli_cursor; i--) {
        cli_line_buf[i] = cli_line_buf[i - 1];
    }
    
    cli_line_buf[cli_cursor] = c;
    cli_line_idx++;
    cli_cursor++;
    
    cliPrintf("%c", c);
    if (cli_cursor < cli_line_idx) {
        cliRedrawTail();
    }
}

static void handleArrowKeys(uint8_t rx_data)
{
    if (rx_data == 'A' || rx_data == 'B') 
    {
        for(int i = cli_cursor; i < cli_line_idx; i++) cliPrintf("\x1B[C");
        cli_cursor = cli_line_idx; // 동기화
        
        if (rx_data == 'A') // 위 화살표
        {
            if (cliHistoryGetPrev(cli_line_buf)) 
            {
                for(int i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");
                
                cli_line_idx = strlen(cli_line_buf);
                cli_cursor = cli_line_idx;
                cliPrintf("%s", cli_line_buf);
            }
        }
        else if (rx_data == 'B') // 아래 화살표
        {
            if (cliHistoryGetNext(cli_line_buf))
            {
                for(int i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");
                
                cli_line_idx = strlen(cli_line_buf);
                cli_cursor = cli_line_idx;
                cliPrintf("%s", cli_line_buf);
            }
        }
    }
    else if (rx_data == 'D') // 왼쪽 화살표
    {
        if (cli_cursor > 0) {
            cli_cursor--;
            cliPrintf("\x1B[D"); 
        }
    }
    else if (rx_data == 'C') // 오른쪽 화살표
    {
        if (cli_cursor < cli_line_idx) {
            cli_cursor++;
            cliPrintf("\x1B[C"); 
        }
    }
}

static void processAnsiEscape(uint8_t rx_data)
{
    if (input_state == CLI_STATE_ESC_RCVD) 
    {
        if (rx_data == '[') input_state = CLI_STATE_BRACKET_RCVD;
        else input_state = CLI_STATE_NORMAL;
    }
    else if (input_state == CLI_STATE_BRACKET_RCVD) 
    {
        handleArrowKeys(rx_data);
        input_state = CLI_STATE_NORMAL; 
    }
}

void cliMain(void)
{
    if (uartAvailable(0) == 0) return;
    
    uint8_t rx_data = uartRead(0);
    
    // 1. 방향키 (ANSI 이스케이프) 파싱 중일 때
    if (input_state != CLI_STATE_NORMAL) 
    {
        processAnsiEscape(rx_data);
        return;
    }
    
    // 2. 일반 문자 입력 처리 (Normal State)
    switch (rx_data) 
    {
        case 0x1B: // ESC 시작
            input_state = CLI_STATE_ESC_RCVD;
            break;
            
        case '\r': // ENTER 키
            handleEnterKey();
            break;
            
        case '\b': // 백스페이스
        case 127:
            handleBackspace();
            break;
            
        default:   // 일반 텍스트 타자
            if (rx_data >= 32 && rx_data <= 126) 
            {
                handleCharInsert(rx_data);
            }
            break;
    }
}




// CLI 전체 초기화 함수
void cliInit(void)
{
    // 버퍼 초기화
    cli_line_idx = 0;
    cli_cursor = 0;
    
    // 하위 모듈들 초기화
    cliHistoryInit();
    cliParserInit(); 
}