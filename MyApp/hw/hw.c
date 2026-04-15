#include "hw.h"


void hwInit(void){
    ledInit();
    uartInit();
    buttonInit();
    tempInit();

}
