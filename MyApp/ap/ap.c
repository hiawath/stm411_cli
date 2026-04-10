#include "ap.h"
#include "hw.h"
#include "hw_def.h"
#include <string.h>

void cliLed(uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            ledOn();
            cliPrintf("LED ON\r\n");
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            ledOff();
            cliPrintf("LED OFF\r\n");
        }
        else if (strcmp(argv[1], "toggle") == 0)
        {
            ledToggle();
            cliPrintf("LED TOGGLED\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: led [on|off|toggle]\r\n");
    }
}
void cliInfo(uint8_t argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "uptime") == 0)
    {
        cliPrintf("System Uptime: %d ms\r\n", millis());
    }
    else
    {
        cliPrintf("info uptime\r\n");
    }
}
void cliSys(uint8_t argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "reset") == 0)
    {
        cliPrintf("System Reset\r\n");
        NVIC_SystemReset(); // ARM Cortex-M 자체 리셋 함수
    }
    else
    {
        cliPrintf("Usage: sys [reset]\r\n");
    }
}


void apInit(void)
{
    cliInit(); // CLI 엔진 기본 세팅
    cliAdd("led", cliLed); // "터미널에서 led 치면 cliLed 함수 실행해 줘" 등록
    cliAdd("info", cliInfo);
    cliAdd("sys", cliSys);
}

void apMain(void)
{
    // 무한 루프
    while(1)
    {
        // ap.c는 키보드가 눌렸는지, 버퍼가 몇 개인지 관심이 없습니다.
        // 그냥 매 루프마다 cli 모듈에게 "너 할일 해!" 라며 함수만 넘겨줍니다.
        cliMain(); 
        
        // 만약 CLI 외에 CAN 통신이나 모터 제어가 있다면 여기에 추가
        // canMain();
        // motorMain();
    }
}
