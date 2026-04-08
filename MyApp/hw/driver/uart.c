#include "uart.h"
#include <stdarg.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

// 링버퍼 크기 설정
#define UART_RX_BUF_LENGTH 256

static uint8_t rx_buf[UART_RX_BUF_LENGTH]; // 데이터를 담을 원형 버퍼
static volatile uint32_t rx_buf_head = 0; // 데이터를 넣을 위치(인터럽트가 이동)
static volatile uint32_t rx_buf_tail =
    0;                  // 데이터를 뺄 위치(어플리케이션이 이동)
static uint8_t rx_data; // 인터럽트로 1바이트씩 받을 임시 변수

bool uartInit(void) {
  // CubeMX가 만들어준 HAL_UART_Init(&huart2)는 이미 main.c에서 완료된
  // 상태입니다. 기본적으로 채널 0(UART2)를 9600bps로 설정하여 엽니다. (수신
  // 인터럽트도 여기서 켜짐)
  //HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  return uartOpen(0, 9600);
}

bool uartOpen(uint8_t ch, uint32_t baud) {
  if (ch == 0) {
    // 현재 보드레이트와 다를 경우에만 재초기화
    if (huart2.Init.BaudRate != baud) {
      huart2.Init.BaudRate = baud;

      // UART 하드웨어 비활성화 후 새로운 설정으로 재초기화
      if (HAL_UART_DeInit(&huart2) != HAL_OK) {
        return false;
      }
      if (HAL_UART_Init(&huart2) != HAL_OK) {
        return false;
      }

      // 수신 인터럽트 다시 활성화
      HAL_UART_Receive_IT(&huart2, &rx_data, 1);
    }
  }

  return true;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length) {
  if (ch == 0) // 논리 채널 0 (또는 _DEF_UART1) 을 UART2로 맵핑
  {
    if (HAL_UART_Transmit(&huart2, p_data, length, 100) == HAL_OK)
      return length;
  }
  return 0;
}

uint32_t uartPrintf(uint8_t ch, char *fmt, ...) {
  char buf[256];
  va_list args;
  int len;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);
  va_end(args);

  return uartWrite(ch, (uint8_t *)buf, len);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  // 어떤 UART 채널로 인터럽트가 발생했는지 확인
  if (huart->Instance == USART2) {
    // 1. 수신받은 1바이트 데이터를 링 버퍼의 head 위치에 저장
    rx_buf[rx_buf_head] = rx_data;

    // 2. head 위치를 한 칸 이동시키고, 버퍼 끝에 도달하면 0으로 되돌리기(Wrap
    // around)
    rx_buf_head = (rx_buf_head + 1) % UART_RX_BUF_LENGTH;

    // 3. 다음 데이터 수신을 위해 인터럽트 다시 재활성화 (중요!)
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    // 에러 이후에도 멈추지 않고 다시 수신 대기 상태로 복구
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  }
}

// 수신된 데이터가 있는지 개수를 반환 (head와 tail의 거리를 계산)
uint32_t uartAvailable(uint8_t ch) {
  uint32_t ret = 0;

  // 버퍼의 head와 tail 위치가 다르면 데이터가 들어온 것
  if (rx_buf_head != rx_buf_tail) {
    if (rx_buf_head > rx_buf_tail)
      ret = rx_buf_head - rx_buf_tail;
    else
      ret = UART_RX_BUF_LENGTH - rx_buf_tail + rx_buf_head;
  }

  return ret;
}

// 링버퍼 맨 앞(tail)에서 데이터를 하나 꺼내오기
uint8_t uartRead(uint8_t ch) {
  uint8_t ret = 0;

  // 읽어올 데이터가 있다면
  if (rx_buf_tail != rx_buf_head) {
    ret = rx_buf[rx_buf_tail]; // tail 위치의 글자를 읽음
    rx_buf_tail = (rx_buf_tail + 1) % UART_RX_BUF_LENGTH; // tail 이동
  }

  return ret;
}
