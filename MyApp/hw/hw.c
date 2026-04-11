#include "hw.h"
#include "cli.h"
#include "hw_def.h"
#include "uart.h"


void hwInit(void){
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
}
