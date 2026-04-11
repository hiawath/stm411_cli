#include "button.h"
#include "cli.h"

static bool is_enabled = false;

void buttonInit(void)
{
    is_enabled = false;
}

void buttonEnable(bool enable)
{
    is_enabled = enable;
}

bool buttonGetEnable(void)
{
    return is_enabled;
}

// HAL Driver의 Weak 콜백 함수 재정의
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (is_enabled == false) return;

    // main.h에 정의된 B1_Pin (GPIO_PIN_13) 확인
    if (GPIO_Pin == B1_Pin)
    {
        cliPrintf("\r\n[Button] B1 Pressed!\r\nCLI> ");
    }
}
