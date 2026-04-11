#include "my_gpio.h"

// 번호(0~7)를 받아 HAL GPIO_TypeDef 포인터로 변환 (0=A, 1=B ...)
static GPIO_TypeDef* getPortPtr(uint8_t port_idx)
{
    switch(port_idx) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        case 7: return GPIOH; // STM32F411은 보통 A~E, H 핀이 있습니다
        default: return NULL;
    }
}

bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state)
{
    if (pin_num > 15) return false;
    
    GPIO_TypeDef* pPort = getPortPtr(port_idx);
    if (pPort == NULL) return false;
    
    uint16_t pin_mask = (1 << pin_num);
    HAL_GPIO_WritePin(pPort, pin_mask, (state > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    return true;
}

uint8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num)
{
    if (pin_num > 15) return -1; // 에러
    
    GPIO_TypeDef* pPort = getPortPtr(port_idx);
    if (pPort == NULL) return -1; // 에러
    
    uint16_t pin_mask = (1 << pin_num);
    return HAL_GPIO_ReadPin(pPort, pin_mask) == GPIO_PIN_SET ? 1 : 0;
}
