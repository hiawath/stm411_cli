#include "monitor.h"

#define LOG_TAG "MON"


typedef enum {
    MONITOR_MODE_OFF = 0,
    MONITOR_MODE_BINARY,
    MONITOR_MODE_ASCII
} monitor_mode_t;

static monitor_packet_t g_packet;
static osMutexId_t monitor_mtx = NULL;
static monitor_mode_t current_monitor_mode = MONITOR_MODE_OFF;
static monitor_sync_cb_t sync_handler = NULL;

// 체크섬 계산용 단순 8비트 XOR
static uint8_t calcChecksum(uint8_t *data, uint32_t len) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

static uint32_t monitor_period = 1000;

// ==========================================
// CLI 제어
// ==========================================
static void cliMonitor(uint8_t argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            monitor_mode_t next_mode = MONITOR_MODE_ASCII; // 기본은 ASCII
            uint32_t next_period = monitor_period;

            if (argc >= 3)
            {
                // 숫자가 바로 오면 ASCII 모드의 주기로 간주
                next_period = atoi(argv[2]);
            }

            if (next_period < 10)
                next_period = 10; // 최소 10ms 제한

            current_monitor_mode = next_mode;
            monitor_period = next_period;

            if (sync_handler != NULL)
                sync_handler(monitorGetPeriod());
            cliPrintf("All tasks synchronized to monitor period: %d ms\r\n", monitor_period);

            cliPrintf("Monitoring Mode: ON (%s)\r\n",
                      (current_monitor_mode == MONITOR_MODE_ASCII) ? "ASCII" : "Binary");
            cliPrintf("Monitoring Period: %d ms\r\n", monitor_period);
            return;
        }
        else if (strcmp(argv[1], "sync") == 0)
        {
            if (sync_handler != NULL)
                sync_handler(monitorGetPeriod());
            cliPrintf("All tasks synchronized to monitor period: %d ms\r\n", monitor_period);
            return;
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            current_monitor_mode = MONITOR_MODE_OFF;
            cliPrintf("Monitoring Mode: OFF (Text Mode Restored)\r\n");
            return;
        }
    }

    cliPrintf("Usage: monitor on [period_ms]\r\n");
    cliPrintf("       monitor on bin [period_ms]\r\n");
    cliPrintf("       monitor off\r\n");
    cliPrintf("       monitor sync\r\n");

    if (current_monitor_mode != MONITOR_MODE_OFF)
    {
        cliPrintf("Current Mode: ON (%s), Period: %d ms\r\n",
                  (current_monitor_mode == MONITOR_MODE_ASCII) ? "ASCII" : "Binary",
                  monitor_period);
    }
    else
    {
        cliPrintf("Current Mode: OFF (Text)\r\n");
    }
}

// ==========================================
// 시스템 API
// ==========================================
void monitorInit(void) {
    memset(&g_packet, 0, sizeof(g_packet));
    
    if (monitor_mtx == NULL) {
        monitor_mtx = osMutexNew(NULL);
    }
    
    cliAdd("monitor", cliMonitor);
}

void monitorOff(void){
    current_monitor_mode=MONITOR_MODE_OFF;
}
bool isMonitoringOn(void) {
    return (current_monitor_mode != MONITOR_MODE_OFF);
}

void monitorSetSyncHandler(monitor_sync_cb_t handler) {
    sync_handler = handler;
}

uint32_t monitorGetPeriod(void) {
    return monitor_period;
}

void monitorSetPeriod(uint32_t period) {
    monitor_period = period;
}

void monitorUpdateValue(SensorID id, DataType type, void *p_val) {
    if (monitor_mtx == NULL) return;
    
    osMutexAcquire(monitor_mtx, osWaitForever);
    
    // 1. 이미 등록된 센서 ID가 있는지 탐색
    int target_idx = -1;
    for (int i = 0; i < g_packet.count; i++) {
        if (g_packet.nodes[i].id == (uint8_t)id) {
            target_idx = i;
            break;
        }
    }
    
    // 2. 새로운 센서라면 끝에 추가
    if (target_idx == -1) {
        if (g_packet.count < MAX_SENSOR_NODES) {
            target_idx = g_packet.count;
            g_packet.nodes[target_idx].id = (uint8_t)id;
            g_packet.count++;
        } else {
            osMutexRelease(monitor_mtx);
            return; // 꽉 참
        }
    }
    
    // 3. 타입 갱신 및 값 복사 (4바이트 고정)
    g_packet.nodes[target_idx].type = (uint8_t)type;
    
    if (type == TYPE_UINT8 || type == TYPE_BOOL) {
        // 1바이트 데이터는 공간의 첫 번째 바이트에 저장
        g_packet.nodes[target_idx].value.u8_val[0] = *(uint8_t*)p_val;
    } else {
        // float, int32, uint32 등 4바이트 데이터 복사
        memcpy(&g_packet.nodes[target_idx].value, p_val, 4);
    }
    
    osMutexRelease(monitor_mtx);
}

void monitorSendPacket(void) {
    if (current_monitor_mode == MONITOR_MODE_OFF || monitor_mtx == NULL) return;
    
    osMutexAcquire(monitor_mtx, osWaitForever);
    
    if (current_monitor_mode == MONITOR_MODE_BINARY) {
        // 기존 바이너리 전송 로직
        uint8_t tx_buf[256]; 
        uint32_t len = 0;
        
        tx_buf[len++] = MONITOR_STX;
        
        uint8_t count = g_packet.count;
        tx_buf[len++] = count;
        
        // [ID(1)][TYPE(1)][VALUE(4)] 복사
        for (int i = 0; i < count; i++) {
            tx_buf[len++] = g_packet.nodes[i].id;
            tx_buf[len++] = g_packet.nodes[i].type;
            tx_buf[len++] = g_packet.nodes[i].value.u8_val[0];
            tx_buf[len++] = g_packet.nodes[i].value.u8_val[1];
            tx_buf[len++] = g_packet.nodes[i].value.u8_val[2];
            tx_buf[len++] = g_packet.nodes[i].value.u8_val[3];
        }
        
        // 체크섬 계산 (Count부터 노드 끝까지 XOR)
        uint8_t crc = calcChecksum(&tx_buf[1], len - 1);
        tx_buf[len++] = crc;
        tx_buf[len++] = MONITOR_ETX;
        
        uartWrite(0, tx_buf, len); 
    } 
    else if (current_monitor_mode == MONITOR_MODE_ASCII) {
        // 아스키 모드 전송 로직
        char tx_buf[512] = {0};
        int len = 0;
        
        // 시작 문자 $ 및 데이터 개수(Count) 전송
        len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "$%d", g_packet.count);
        
        for (int i = 0; i < g_packet.count; i++) {
            uint8_t id = g_packet.nodes[i].id;
            uint8_t type = g_packet.nodes[i].type;
            
            // 데이터 구분자 추가
            len += snprintf(tx_buf + len, sizeof(tx_buf) - len, ",");
            
            len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%d:%d:", id, type);
            
            // 타입별 데이터 포매팅
            if (type == TYPE_UINT8 || type == TYPE_BOOL) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%u", g_packet.nodes[i].value.u8_val[0]);
            } else if (type == TYPE_INT32) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%ld", (long)g_packet.nodes[i].value.i_val);
            } else if (type == TYPE_FLOAT) {
                // 참고: printf에서 %f가 제대로 동작하려면 Makefile/CMake에 -u _printf_float 옵션이 있어야 함
                // 만약 없다면 정수부/소수부로 쪼개서 출력해야 할 수도 있습니다. 
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%.2f", g_packet.nodes[i].value.f_val);
            } else {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%lu", (unsigned long)g_packet.nodes[i].value.u_val);
            }
        }
        
        // 정지 문자 # 및 개행
        len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "#\r\n");
        uartWrite(0, (uint8_t*)tx_buf, len);
    }
    
    osMutexRelease(monitor_mtx);
}
