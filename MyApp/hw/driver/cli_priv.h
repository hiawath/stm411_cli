#ifndef SRC_HW_DRIVER_CLI_PRIV_H_
#define SRC_HW_DRIVER_CLI_PRIV_H_

#include "hw_def.h"

// ============================================
// CLI 엔진 내부 전용 (Private) 설정값 및 자료구조
// ============================================

#define CLI_LINE_BUF_MAX      256  
#define CLI_CMD_LIST_MAX      32
#define CLI_CMD_ARG_MAX       4
#define CLI_HIST_MAX          10   // 방향키 위/아래 기억 개수

// 명령어 매핑 자료구조
typedef struct {
    char cmd_str[16];
    void (*cmd_func)(uint8_t argc, char **argv);
} cli_cmd_t;

// 터미널 ANSI Escape 시퀀스 (방향키) 파싱을 위한 상태 머신
typedef enum {
    CLI_STATE_NORMAL = 0,      // 일반 타자 대기
    CLI_STATE_ESC_RCVD,        // ESC [ 수신
    CLI_STATE_BRACKET_RCVD     // 방향키(A,B,C,D) 판독 전
} cli_input_state_t;

#endif /* SRC_HW_DRIVER_CLI_PRIV_H_ */
