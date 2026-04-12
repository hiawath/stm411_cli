#include "temp.h"
#include "adc.h" // CubeMX 자동 생성 헤더 (hadc1 선언 포함)

// DMA가 지속적으로 값을 갱신할 버퍼 (값 1개)
static volatile uint32_t adc_dma_buf[1] = {0};

bool tempInit(void)
{
    // 초기에는 전력 소모를 막기 위해 DMA를 켜지 않습니다.
    return true;
}

void tempStartAuto(void)
{
    // DMA Continuous 모드 시작
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, 1);
}

void tempStopAuto(void)
{
    // DMA 완전 정지 (ADC 전력 절약)
    HAL_ADC_Stop_DMA(&hadc1);
}

float tempReadAuto(void)
{
    uint32_t adc_val = adc_dma_buf[0];
    float vsense = ((float)adc_val / 4095.0f) * 3.3f;
    return ((vsense - 0.76f) / 0.0025f) + 25.0f;
}

float tempReadSingle(void)
{
    uint32_t adc_val = 0;
    
    // DMA 없이 Polling 방식으로 딱 1회 전력을 소모하여 읽음
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        adc_val = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    
    float vsense = ((float)adc_val / 4095.0f) * 3.3f;
    return ((vsense - 0.76f) / 0.0025f) + 25.0f;
}
