#include "uart.h"
#include <stdarg.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

bool uartInit(void)
{
    return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
    // baud 변경 등은 생략하고 기본 통신만 열어둠
    return true;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
    if (ch == 0) // 논리 채널 0 (또는 _DEF_UART1) 을 UART2로 맵핑
    {
        if (HAL_UART_Transmit(&huart2, p_data, length, 100) == HAL_OK) return length;
    }
    return 0;
}

uint32_t uartPrintf(uint8_t ch, char *fmt, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    return uartWrite(ch, (uint8_t *)buf, len);
}
