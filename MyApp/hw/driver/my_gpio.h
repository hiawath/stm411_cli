#ifndef _SRC_HW_DRIVER_GPIO_H_
#define _SRC_HW_DRIVER_GPIO_H_

#include "hw_def.h"

// 포트 번호: 0=A, 1=B, 2=C ... 7=H
bool gpioExtWrite(uint8_t port_idx, uint8_t pin_num, uint8_t state);
uint8_t gpioExtRead(uint8_t port_idx, uint8_t pin_num); // 정상시 0 또는 1, 에러시 -1 반환

#endif
