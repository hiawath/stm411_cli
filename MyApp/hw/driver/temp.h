#ifndef SRC_HW_DRIVER_TEMP_H_
#define SRC_HW_DRIVER_TEMP_H_

#include "hw_def.h"

bool tempInit(void);

// 단발성 읽기 (필요할 때만 ADC 전원 켬)
float tempReadSingle(void);

// DMA Continuous (자동 갱신) 모드 제어
void tempStartAuto(void);
void tempStopAuto(void);

// DMA 백그라운드 버퍼에서 값 꺼내오기
float tempReadAuto(void);

#endif /* SRC_HW_DRIVER_TEMP_H_ */
