#include "bsp.h"
#include "main.h"

void bspInit(void)
{
    // 추후 시스템 셋팅이 필요하면 여기 추가
}

void delay(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t millis(void)
{
    return HAL_GetTick();
}
