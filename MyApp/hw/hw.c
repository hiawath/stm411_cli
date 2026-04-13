#include "hw.h"
#include "cli.h"
#include "hw_def.h"
#include "uart.h"
#include "log.h"


void hwInit(void){
    ledInit();
    uartInit();
    buttonInit();
    tempInit();

}
