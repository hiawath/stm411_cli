#define LOG_TAG "AP"

#include "ap.h"



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
            if (state < 0)
            {
                cliPrintf("Invalid Port or Pin (ex: a5, b12)\r\n");
            }
            else
            {
                cliPrintf("GPIO %c%d = %d\r\n", toupper(port_char), pin_num, state);
            }
        }
        else if (strcmp(argv[1], "write") == 0 && argc == 4)
        {
            int val = atoi(argv[3]);
            if (gpioExtWrite(port_idx, pin_num, val))
            {
                cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val ? 1 : 0);
            }
            else
            {
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
    if (addr >= 0x08000000 && addr <= 0x0807FFFF)
        return true;

    // 2. STM32F411 SRAM (보통 128KB: 0x2000 0000 ~ 0x2001 FFFF)
    if (addr >= 0x20000000 && addr <= 0x2001FFFF)
        return true;

    // 3. System Memory 영역 (부트로더 및 공장 출고 UID 영역 등)
    if (addr >= 0x1FFF0000 && addr <= 0x1FFF7A1F)
        return true;

    // 4. Peripheral 레지스터 영역 (0x40000000 부터 시작)
    // 주의: 유효한 영역이라도 클럭이 꺼져있으면 Bus Fault가 나지만, 허용은
    // 해둡니다.
    if (addr >= 0x40000000 && addr <= 0x5FFFFFFF)
        return true;

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

        if (argc >= 3)
        {
            length = strtoul(argv[2], NULL, 0);
        }

        // 16바이트씩 단위로 루프
        for (uint32_t i = 0; i < length; i += 16)
        {
            cliPrintf("0x%08X : ", addr + i);

            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length)
                {
                    uint32_t target_addr = addr + i + j;

                    // Bus Fault 방지용 메모리 맵 유효성 검사
                    if (isSafeAddress(target_addr))
                    {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        cliPrintf("%02X ", val);
                    }
                    else
                    {
                        // 읽기 금지 구역은 ?? 로 출력
                        cliPrintf("?? ");
                    }
                }
                else
                {
                    cliPrintf("   ");
                }
            }

            cliPrintf(" | ");

            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length)
                {
                    uint32_t target_addr = addr + i + j;
                    if (isSafeAddress(target_addr))
                    {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        if (val >= 32 && val <= 126)
                            cliPrintf("%c", val);
                        else
                            cliPrintf(".");
                    }
                    else
                    {
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

static uint32_t led_toggle_period = 0; // 0이면 자동 점멸 중지 상태

void cliLed(uint8_t argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            led_toggle_period = 0; // 동작 중지
            ledOn();
            cliPrintf("LED ON\r\n");
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            led_toggle_period = 0; // 동작 중지
            ledOff();
            cliPrintf("LED OFF\r\n");
        }
        else if (strcmp(argv[1], "toggle") == 0)
        {
            if (argc == 3)
            {
                // 주기적인 자동 토글 모드 (예: led toggle 1000)
                int period = atoi(argv[2]);
                if (period > 0)
                {
                    led_toggle_period = period;
                    cliPrintf("LED Auto-Toggle Started (%d ms)\r\n", period);
                }
                else
                {
                    cliPrintf("Invalid Period\r\n");
                }
            }
            else
            {
                // 단발성 토글 모드 (예: led toggle)
                led_toggle_period = 0; // 기존의 자동 점멸 태스크를 멈춤
                ledToggle();
                cliPrintf("LED TOGGLED ONCE\r\n");
            }
        }
        // LED 상태 즉시 업데이트
        bool led_state = ledGetStatus();
        monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);
    }
    else
    {
        cliPrintf("Usage: led [on|off]\r\n");
        cliPrintf("       led toggle\r\n");
        cliPrintf("       led toggle [period_ms]\r\n");
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
        uint32_t uid0 = *(volatile uint32_t *)0x1FFF7A10;
        uint32_t uid1 = *(volatile uint32_t *)0x1FFF7A14;
        uint32_t uid2 = *(volatile uint32_t *)0x1FFF7A18;
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

void cliButton(uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            buttonEnable(true);
            cliPrintf("Button Interrupt Report: ON\r\n");
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            buttonEnable(false);
            cliPrintf("Button Interrupt Report: OFF\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: button [on/off]\r\n");
        cliPrintf("Current Status: %s\r\n", buttonGetEnable() ? "ON" : "OFF");
    }
}

static uint32_t temp_read_period = 0; // 주기(ms) 0이면 주기적 동작 멈춤

void cliTemp(uint8_t argc, char **argv)
{
    if (argc == 1)
    {
        // 1회 단독 측정 수행 (자동 모드 끄기)
        if (temp_read_period > 0)
        {
            tempStopAuto(); // 기존 자동 모드가 켜져있었다면 DMA 끄기
        }
        temp_read_period = 0;

        float t = tempReadSingle(); // 1회만 Polling으로 전력 아껴서 읽음
        cliPrintf("Current Temp: %.2f *C\r\n", t);
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "off") == 0 || strcmp(argv[1], "stop") == 0)
        {
            temp_read_period = 0;
            tempStopAuto();
            cliPrintf("Temperature Auto-Read Stopped\r\n");
            return;
        }

        int period = atoi(argv[1]);
        if (period > 0)
        {
            if (temp_read_period == 0)
            {
                tempStartAuto(); // DMA Continuous 모드 가동!
            }
            temp_read_period = period;
            cliPrintf("Temperature Auto-Read Started (%d ms)\r\n", period);
        }
        else
        {
            cliPrintf("Invalid Period\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: temp\r\n");
        cliPrintf("       temp [period_ms]\r\n");
        cliPrintf("       temp off\r\n");
    }
}

void apStopAutoTasks(void)
{
    led_toggle_period = 0;
    temp_read_period = 0;
    tempStopAuto();
    ledOff();
}

void apInit(void)
{
    LOG_INF("Application Init... Started");
    cliInit(); // CLI 엔진 기본 세팅
    logInit();
    monitorInit(); // 모니터링 시스템 스레드 락 및 변수 초기화

    cliAdd("led", cliLed); // "터미널에서 led 치면 cliLed 함수 실행해 줘" 등록
    cliAdd("info", cliInfo);
    cliAdd("sys", cliSys);
    cliAdd("gpio", cliGpio);     // GPIO 읽기/쓰기 커맨드 등록
    cliAdd("md", cliMd);         // 메모리 덤프 커맨드 등록
    cliAdd("button", cliButton); // 버튼 보고 제어 등록
    cliAdd("temp", cliTemp);     // 온도 센서 명령 등록
}


void StartDefaultTask(void *argument)
{
    hwInit();
    apInit();
    while (1)
    {
        apMain();
    }
}

// 📌 백그라운드 시스템 태스크 (LED 점멸 등)
void ledSystemTask(void *argument)
{
    while (1)
    {
        if (led_toggle_period > 0)
        {
            LOG_DBG("LED Toggle!");
            ledToggle();

            bool led_state = ledGetStatus();
            monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);

            osDelay(led_toggle_period);
        }
        else
        {
            bool led_state = ledGetStatus();
            monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);
            osDelay(100);
        }
    }
}

// 📌 백그라운드 온도 센서 태스크
void tempSystemTask(void *argument)
{
    while (1)
    {
        if (temp_read_period > 0)
        {
            // 이미 켜진 DMA 버퍼에서 값만 쏙 빼와서 출력 (Zero Overhead)
            float t = tempReadAuto();

            // 시나리오 로그 적용
            LOG_DBG("Temp Sensor DMA Buffer Read Success");

            if (t > 40.0f)
            {
                LOG_WRN("High Temperature Alert: %.2f *C", t);
            }

            // 모니터 시스템에 현재 온도값을 갱신 (전략 1)
            monitorUpdateValue(ID_ENV_TEMP, TYPE_FLOAT, &t);

            // 사람을 위한 텍스트 모드일 때만 출력
            if (!isMonitoringOn())
            {
                cliPrintf("Current Temp: %.2f *C\r\n", t);
            }

            osDelay(temp_read_period);
        }
        else
        {
            osDelay(50);
        }
    }
}

// 📌 20ms 고속 모니터링 전용 송신 태스크 (전략 2)
void monitorSystemTask(void *argument)
{
    while (1)
    {
        if (isMonitoringOn())
        {
            // 현재 스냅샷에 찍힌 모든 센서들의 값을 TLV 패킷으로 조립하여 UART로 고속
            // 블라스트!
            monitorSendPacket();
        }
        osDelay(2000); // 20ms 주기 (50Hz)
    }
}

void apMain(void)
{

    while (1)
    {
        // cliMain 내부에서 큐를 이용해 무한 대기(Blocking)하므로 CPU 점유율이 0%가
        // 됩니다. 따라서 기존에 있던 osDelay()는 삭제해도 안전합니다.
        cliMain();
    }
}
