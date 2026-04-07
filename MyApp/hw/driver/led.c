#include "led.h"
#include "bsp.h"
#include "hw_def.h"


void ledInit(void)
{
    delay(10);
}

void ledOn(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
}

void ledOff(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

void ledToggle(void)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}
