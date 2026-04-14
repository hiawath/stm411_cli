#include "monitor.h"
#include "cmsis_os.h"
#include "cli.h"
#include "uart.h"
#include <string.h>

#define LOG_TAG "MON"
#include "log.h"

// 글로벌 모니터 저장소 및 플래그
static monitor_packet_t g_packet;
static osMutexId_t monitor_mtx = NULL;
static bool is_monitoring_active = false;

// 체크섬 계산용 단순 8비트 XOR
static uint8_t calcChecksum(uint8_t *data, uint32_t len) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

// ==========================================
// CLI 제어
// ==========================================
static void cliMonitor(uint8_t argc, char **argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "on") == 0) {
            is_monitoring_active = true;
            // 텍스트 출력 마지막 메시지 (모니터 모드로 넘어가면 텍스트를 볼 수 없음)
            cliPrintf("Monitoring Mode: ON (Text logs will be suppressed)\r\n");
            return;
        } 
        else if (strcmp(argv[1], "off") == 0) {
            is_monitoring_active = false;
            cliPrintf("Monitoring Mode: OFF (Text Mode Restored)\r\n");
            return;
        }
    }
    
    cliPrintf("Usage: monitor [on|off]\r\n");
    cliPrintf("Current Mode: %s\r\n", is_monitoring_active ? "ON (Binary)" : "OFF (Text)");
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

bool isMonitoringOn(void) {
    return is_monitoring_active;
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
    if (!is_monitoring_active || monitor_mtx == NULL) return;
    
    osMutexAcquire(monitor_mtx, osWaitForever);
    
    // 패킷 조립을 위한 버퍼 생성.
    // 최대 용량 = STX(1) + Count(1) + Nodes(MAX*6) + CRC(1) + ETX(1)
    //02 01 0A 02 D6 2F C5 41 74 03
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
    
    // 자물쇠를 풀기 전에 전송해버리는게 데이터를 보장함 (UART MUTEX가 내부에 있어 안전)
    uartWrite(0, tx_buf, len); 
    
    osMutexRelease(monitor_mtx);
}
