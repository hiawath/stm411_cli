#include "temp.h"
#include "adc.h" // CubeMX 자동 생성 헤더 (hadc1 선언 포함)

// DMA가 지속적으로 값을 갱신할 버퍼 (값 1개)
static volatile uint32_t adc_dma_buf[1];

bool tempInit(void)
{
    // CubeMX가 초기화한 ADC1을 기반으로, DMA 연동 모드를 백그라운드에서 백그라운드에서 영구 구동시킵니다.
    // Continuous 모드와 Circular DMA 모드가 섞여있으므로, CPU 개입 없이 adc_dma_buf[0]에 값이 실시간 갱신됩니다.
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, 1);
    return true;
}

float tempRead(void)
{
    // 1. DMA가 백그라운드에서 실시간으로 갱신해둔 데이터(0~4095)를 딜레이 없이 획득
    uint32_t adc_val = adc_dma_buf[0];
    
    // 2. STM32F411 온도 공식 적용
    float vsense = ((float)adc_val / 4095.0f) * 3.3f;
    float temp_celsius = ((vsense - 0.76f) / 0.0025f) + 25.0f;
    
    // 블로킹(Polling) 로직 제거됨! 오버헤드 0%
    return temp_celsius;
}
