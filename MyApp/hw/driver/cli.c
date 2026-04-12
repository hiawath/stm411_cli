// MyApp/hw/driver/cli.c
#include "cli.h"
#include "uart.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h> // strncpy를 사용하기 위해 필요

#define CLI_LINE_BUF_MAX      256  

#define CLI_CMD_LIST_MAX      32
#define CLI_CMD_ARG_MAX       4

typedef struct {
    char cmd_str[16];
    void (*cmd_func)(uint8_t argc, char **argv);
} cli_cmd_t;

// 등록된 커맨드들을 담아둘 배열
static cli_cmd_t cli_cmd_list[CLI_CMD_LIST_MAX]; 
static uint8_t   cli_cmd_count = 0;

// 파싱이 끝난 인자를 담보할 배열 (ex: argv[0]="led", argv[1]="on")
static uint8_t   cli_argc = 0; 
static char*     cli_argv[CLI_CMD_ARG_MAX]; 
static char      cli_line_buf[CLI_LINE_BUF_MAX]; 
static uint16_t  cli_line_idx = 0; // 총 담긴 글자 수 저장
static uint16_t  cli_cursor = 0;   // 현재 깜빡이는 논리적 커서 위치

#define CLI_HIST_MAX 10
static char      cli_hist_buf[CLI_HIST_MAX][CLI_LINE_BUF_MAX]; // 10개까지 기억하는 2차원 히스토리 버퍼
static uint8_t   cli_hist_count = 0;   // 현재 저장된 히스토리 개수 (최대 10)
static uint8_t   cli_hist_write = 0;   // 다음 데이터를 저장할 배열 인덱스
static uint8_t   cli_hist_depth = 0;   // 위/아래 방향키를 누를 때 어느 과거를 보고 있는지 추적하는 변수

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

// 예: line_buf가 "led on" 이라면 
void cliParseArgs(char *line_buf)
{
    char *tok;
    
    cli_argc = 0; // 카운트 초기화
    
    // strtok 함수는 공백(" ")을 기준으로 문자열을 잘라 포인터를 반환합니다.
    tok = strtok(line_buf, " ");
    
    while(tok != NULL && cli_argc < CLI_CMD_ARG_MAX)
    {
        cli_argv[cli_argc] = tok; 
        cli_argc++;
        tok = strtok(NULL, " ");  // 다음 단어 찾기
    }
}

// 결과: cli_argc는 2, cli_argv[0]은 "led", cli_argv[1]은 "on"을 가리킵니다.
void cliRunCommand(void)
{
    if (cli_argc == 0) return; // 엔터만 쳤을 경우 그냥 무시
    
    bool is_found = false;

    // 등록된 명령어 개수(cli_cmd_count)만큼 반복하며 이름이 같은지 검사
    for (int i = 0; i < cli_cmd_count; i++)
    {
        if (strcmp(cli_argv[0], cli_cmd_list[i].cmd_str) == 0)
        {
            is_found = true;
            // 일치하는 명령어를 찾았다면, 구조체에 저장된 함수 포인터 실행!
            cli_cmd_list[i].cmd_func(cli_argc, cli_argv);
            break;
        }
    }
    
    // 리스트를 다 뒤졌는데 이름이 일치하는 게 없다면
    if (is_found == false) 
    {
        cliPrintf("Command Not Found\r\n");
    }
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
        
        strncpy(cli_hist_buf[cli_hist_write], cli_line_buf, CLI_LINE_BUF_MAX - 1);
        cli_hist_write = (cli_hist_write + 1) % CLI_HIST_MAX;
        if (cli_hist_count < CLI_HIST_MAX) cli_hist_count++;
        cli_hist_depth = 0; 
        
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
            if (cli_hist_depth < cli_hist_count) 
            {
                cli_hist_depth++; 
                for(int i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");
                
                int idx = (cli_hist_write - cli_hist_depth + CLI_HIST_MAX) % CLI_HIST_MAX;
                strncpy(cli_line_buf, cli_hist_buf[idx], CLI_LINE_BUF_MAX - 1);
                cli_line_idx = strlen(cli_line_buf);
                cli_cursor = cli_line_idx;
                cliPrintf("%s", cli_line_buf);
            }
        }
        else if (rx_data == 'B') // 아래 화살표
        {
            if (cli_hist_depth > 0)
            {
                cli_hist_depth--; 
                for(int i = 0; i < cli_line_idx; i++) cliPrintf("\b \b");
                
                if (cli_hist_depth == 0) {
                    cli_line_buf[0] = '\0';
                    cli_line_idx = 0;
                    cli_cursor = 0;
                } else {
                    int idx = (cli_hist_write - cli_hist_depth + CLI_HIST_MAX) % CLI_HIST_MAX;
                    strncpy(cli_line_buf, cli_hist_buf[idx], CLI_LINE_BUF_MAX - 1);
                    cli_line_idx = strlen(cli_line_buf);
                    cli_cursor = cli_line_idx;
                    cliPrintf("%s", cli_line_buf);
                }
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




bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char **argv))
{
    // 1. 등록할 방이 남아있는지 확인 (최대 개수 방지)
    if (cli_cmd_count >= CLI_CMD_LIST_MAX)
    {
        return false; 
    }

    // 2. 명령어 문자열(이름)을 내부 구조체 배열로 복사 
    // (버퍼 오버플로우를 막기 위해 최대 허용 길이-1 만큼만 복사)
    strncpy(cli_cmd_list[cli_cmd_count].cmd_str, cmd_str, sizeof(cli_cmd_list[0].cmd_str) - 1);
    
    // 3. 콜백할 함수 포인터를 구조체에 연결
    cli_cmd_list[cli_cmd_count].cmd_func = cmd_func;
    
    // 4. 다음 명령어 등록을 위해 카운트 1 증가
    cli_cmd_count++;
    
    return true;
}

static void cliHelp(uint8_t argc, char **argv);
static void cliClear(uint8_t argc, char **argv); // cls 함수 프로토타입 추가

// CLI 전체 초기화 함수
void cliInit(void)
{
    // 1. 변수들 초기화 (시스템 리셋 시 쓰레기값 방지)
    cli_cmd_count = 0;
    cli_line_idx = 0;
    cli_cursor = 0; // 커서 위치 초기화
    
    // 2. 운영체제의 'help' 또는 '?' 처럼, 가장 기본이 되는 명령어를 자동 등록
    cliAdd("help", cliHelp);
    cliAdd("cls", cliClear); // cls 명령어 등록
}
// 기본 제공되는 help 명령어의 실제 동작
static void cliHelp(uint8_t argc, char **argv)
{
    cliPrintf("--------------- CLI Commands ---------------\r\n");
    
    // 지금까지 등록된 모든 명령어 리스트를 터미널에 한 줄씩 뿌려줌
    for (int i = 0; i < cli_cmd_count; i++)
    {
        cliPrintf("  %s\r\n", cli_cmd_list[i].cmd_str);
    }
    
    cliPrintf("--------------------------------------------\r\n");
}

// 화면 지우기 (cls) 실제 동작
static void cliClear(uint8_t argc, char **argv)
{
    // \x1B[2J : 터미널 화면 전체 지우기
    // \x1B[H  : 커서를 맨 위 가장 좌측(Home)으로 이동
    cliPrintf("\x1B[2J\x1B[H");
}