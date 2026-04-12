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

// 매크로(상수)만 유지합니다. 자료구조는 사용되는 .c 파일 내부로 각각 이동합니다.

#endif /* SRC_HW_DRIVER_CLI_PRIV_H_ */
