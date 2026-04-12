#include "uart.h"
#include "cmsis_os.h" // FreeRTOS Message Queue을 위해 추가
#include <stdarg.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

// 큐 핸들
static osMessageQueueId_t uart_rx_q = NULL;

static uint8_t rx_data; // 인터럽트 수신용 1바이트 임시 변수

bool uartInit(void) {
  // FreeRTOS 메세지 큐 생성 (크기 256, 단위 1바이트)
  if (uart_rx_q == NULL) {
      uart_rx_q = osMessageQueueNew(256, sizeof(uint8_t), NULL);
  }
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

      HAL_UART_Transmit(&huart2, (uint8_t *)"UART Reopened\r\n", 16, 100);
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
  if (huart->Instance == USART2) {
    if (uart_rx_q != NULL) {
        // 인터럽트 컨텍스트 내이므로 timeout은 0으로 설정
        osMessageQueuePut(uart_rx_q, &rx_data, 0, 0); 
    }
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    // 에러 이후에도 멈추지 않고 다시 수신 대기 상태로 복구
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);
  }
}

uint32_t uartAvailable(uint8_t ch) {
  // 큐에 들어있는 메시지 개수 반환
  if (ch == 0 && uart_rx_q != NULL) {
      return osMessageQueueGetCount(uart_rx_q);
  }
  return 0;
}

uint8_t uartRead(uint8_t ch) {
  uint8_t ret = 0;
  if (ch == 0 && uart_rx_q != NULL) {
      // 대기 없이 데이터 꺼내기 (Polling 호환성 유지)
      osMessageQueueGet(uart_rx_q, &ret, NULL, 0);
  }
  return ret;
}

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout) {
  if (ch == 0 && uart_rx_q != NULL) {
      if (osMessageQueueGet(uart_rx_q, p_data, NULL, timeout) == osOK) {
          return true;
      }
  }
  return false;
}
