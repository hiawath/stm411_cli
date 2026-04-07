#include "hw.h"
#include "hw_def.h"
#include "uart.h"


void hwInit(void){
    ledInit();
    uartInit();
}
