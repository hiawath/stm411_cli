#ifndef MYAPP_HW_DRIVER_LOG_H_
#define MYAPP_HW_DRIVER_LOG_H_

#include "hw_def.h"

bool logInit(void);
void logSetLevel(uint8_t level);
uint8_t logGetLevel(void);

#endif /* MYAPP_HW_DRIVER_LOG_H_ */
