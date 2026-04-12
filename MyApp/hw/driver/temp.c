#include "temp.h"
#include "adc.h" // CubeMX 자동 생성 헤더 (hadc1 선언 포함)

bool tempInit(void)
{
    // CubeMX가 main.c에서 MX_ADC1_Init()을 호출하여 하드웨어를 이미 초기화했습니다.
    // 추가적인 소프트웨어 필터나 구조체 초기화가 필요하다면 여기서 수행합니다.
    return true;
}

float tempRead(void)
{
    uint32_t adc_val = 0;
    float temp_celsius = 0.0f;
    
    // 1. ADC 변환 1회 수행
    HAL_ADC_Start(&hadc1);
    
    // 2. 변환이 끝날 때까지 대기 (타임아웃 10ms)
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        // 3. 변환된 12bit(0~4095) 원시 데이터 획득
        adc_val = HAL_ADC_GetValue(&hadc1);
        
        // 4. STM32F411 온도 공식 적용
        //   - 동작 전압 통상 3.3V
        //   - 25도일 때의 전압 V25 = 0.76 V (스펙시트 참고)
        //   - 온도 기울기 Avg_Slope = 2.5 mV/C = 0.0025 V/C
        
        float vsense = ((float)adc_val / 4095.0f) * 3.3f;
        temp_celsius = ((vsense - 0.76f) / 0.0025f) + 25.0f;
    }
    
    // 5. ADC 변환 중지
    HAL_ADC_Stop(&hadc1);
    
    return temp_celsius;
}
