#include "ap.h"
#include "hw.h"
#include "hw_def.h"



void apInit(void){
    
}

void apMain(void)
{
    uartPrintf(0, "Welcome to Nucleo CLI FW!\r\n");

    while(1)
    {
        if (uartAvailable(0) > 0)
        {
            uint8_t rx_data = uartRead(0);
            
            // 수신받은 문자 즉시 출력 (에코)
            uartPrintf(0, "%c", rx_data);
        }
    }
}
