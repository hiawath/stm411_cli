#include "ap.h"
#include "hw.h"
#include "hw_def.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void cliGpio(uint8_t argc, char **argv)
{
    if (argc >= 3)
    {
        // argv[1]: "read" 또는 "write"
        // argv[2]: 핀 이름 (예: "a5", "b12")
        char port_char = tolower(argv[2][0]); // 첫 글자를 소문자로 ('a' ~ 'h')
        int pin_num = atoi(&argv[2][1]);      // 두 번째 글자부터는 숫자로 변환 (0~15)
        
        uint8_t port_idx = port_char - 'a'; // 'a'는 0, 'b'는 1 ...
        
        if (strcmp(argv[1], "read") == 0)
        {
            int8_t state = gpioExtRead(port_idx, pin_num);
            if (state < 0) {
                cliPrintf("Invalid Port or Pin (ex: a5, b12)\r\n");
            } else {
                cliPrintf("GPIO %c%d = %d\r\n", toupper(port_char), pin_num, state);
            }
        }
        else if (strcmp(argv[1], "write") == 0 && argc == 4)
        {
            int val = atoi(argv[3]);
            if (gpioExtWrite(port_idx, pin_num, val)) {
                cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val ? 1 : 0);
            } else {
                cliPrintf("Invalid Port or Pin (ex: a5, b12)\r\n");
            }
        }
        else
        {
            cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
            cliPrintf("       gpio write [a~h][0~15] [0/1]\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
        cliPrintf("       gpio write [a~h][0~15] [0/1]\r\n");
    }
}

// 유효한 메모리 영역인지 검사하는 보호 함수
static bool isSafeAddress(uint32_t addr)
{
    // 1. STM32F411 Flash (보통 512KB: 0x0800 0000 ~ 0x0807 FFFF)
    if (addr >= 0x08000000 && addr <= 0x0807FFFF) return true;
    
    // 2. STM32F411 SRAM (보통 128KB: 0x2000 0000 ~ 0x2001 FFFF)
    if (addr >= 0x20000000 && addr <= 0x2001FFFF) return true;
    
    // 3. System Memory 영역 (부트로더 및 공장 출고 UID 영역 등)
    if (addr >= 0x1FFF0000 && addr <= 0x1FFF7A1F) return true;
    
    // 4. Peripheral 레지스터 영역 (0x40000000 부터 시작)
    // 주의: 유효한 영역이라도 클럭이 꺼져있으면 Bus Fault가 나지만, 허용은 해둡니다.
    if (addr >= 0x40000000 && addr <= 0x5FFFFFFF) return true;
    
    // 그 외에 아무것도 매핑되지 않은 주소는 접근 불가 타겟으로 간주하여 차단
    return false;
}

// 메모리 덤프 (Memory Dump) 명령어
void cliMd(uint8_t argc, char **argv)
{
    if (argc >= 2)
    {
        uint32_t addr = strtoul(argv[1], NULL, 16);
        uint32_t length = 16; 
        
        if (argc >= 3) {
            length = strtoul(argv[2], NULL, 0); 
        }
        
        // 16바이트씩 단위로 루프
        for (uint32_t i = 0; i < length; i += 16)
        {
            cliPrintf("0x%08X : ", addr + i);
            
            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length) {
                    uint32_t target_addr = addr + i + j;
                    
                    // Bus Fault 방지용 메모리 맵 유효성 검사
                    if (isSafeAddress(target_addr)) {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        cliPrintf("%02X ", val);
                    } else {
                        // 읽기 금지 구역은 ?? 로 출력
                        cliPrintf("?? ");
                    }
                } else {
                    cliPrintf("   ");
                }
            }
            
            cliPrintf(" | ");
            
            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length) {
                    uint32_t target_addr = addr + i + j;
                    if (isSafeAddress(target_addr)) {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        if (val >= 32 && val <= 126) cliPrintf("%c", val);
                        else cliPrintf(".");
                    } else {
                        cliPrintf(".");
                    }
                }
            }
            cliPrintf("\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: md [addr(hex)] [length]\r\n");
        cliPrintf("       md 08000000 32\r\n");
    }
}

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
    if (argc == 1) // 인자가 없을 때 기본 정보 출력
    {
        cliPrintf("====================================\r\n");
        cliPrintf(" HW Model   : STM32F411\r\n");
        cliPrintf(" FW Version : V1.0.0\r\n");
        cliPrintf(" Build Date : %s %s\r\n", __DATE__, __TIME__);
        
        // STM32 Unique ID 레지스터 주소 (F411 기준)
        uint32_t uid0 = *(volatile uint32_t*)0x1FFF7A10;
        uint32_t uid1 = *(volatile uint32_t*)0x1FFF7A14;
        uint32_t uid2 = *(volatile uint32_t*)0x1FFF7A18;
        cliPrintf(" Serial Num : %08X-%08X-%08X\r\n", uid0, uid1, uid2);
        cliPrintf("====================================\r\n");
    }
    else if (argc == 2 && strcmp(argv[1], "uptime") == 0)
    {
        cliPrintf("System Uptime: %d ms\r\n", millis());
    }
    else
    {
        cliPrintf("Usage: info\r\n");
        cliPrintf("       info uptime\r\n");
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
    cliAdd("gpio", cliGpio); // GPIO 읽기/쓰기 커맨드 등록
    cliAdd("md", cliMd); // 메모리 덤프 커맨드 등록
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
