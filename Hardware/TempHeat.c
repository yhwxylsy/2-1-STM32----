#include "TempHeat.h"

#define ADC_CONVERSION_WAIT_LIMIT 100000U /* 循环上限，实际时长需按目标编译配置校准 */
#define ADC_CALIBRATION_WAIT_LIMIT 100000U /* 校准等待上限，实际时长需按目标编译配置校准 */
#define ADC_ASYNC_WAIT_LIMIT 10U /* 10 ms轮询一次，单次转换最长等待约100 ms。 */

static uint8_t g_adcReady = 0; /* 校准成功后才允许温度采样。 */
static uint16_t g_asyncSamples[20];
static uint8_t g_asyncSampleIndex;
static uint8_t g_asyncSampling;
static uint8_t g_asyncWaitCount;

/**
  * @brief  热敏电阻温度传感器和加热控制初始化
  * @param  无
  * @retval 无
  */
void TempHeat_Init(void) {
    uint32_t waitCount;
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    g_adcReady = 0;
    RCC_APB2PeriphClockCmd(THERMISTOR_RCC | RCC_APB2Periph_ADC1 | HEAT_RCC, ENABLE);

    /* 先建立确定的加热关闭态，再启动可能失败的ADC校准。 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = HEAT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HEAT_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(HEAT_PORT, HEAT_PIN);

    // 初始化热敏电阻模拟输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = THERMISTOR_PIN;
    GPIO_Init(THERMISTOR_PORT, &GPIO_InitStructure);
    
    // 初始化ADC1
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(THERMISTOR_ADC, &ADC_InitStructure);
    
    // 启用ADC1
    ADC_Cmd(THERMISTOR_ADC, ENABLE);
    
    // 校准ADC1
    ADC_ResetCalibration(THERMISTOR_ADC);
    waitCount = ADC_CALIBRATION_WAIT_LIMIT;
    while (ADC_GetResetCalibrationStatus(THERMISTOR_ADC)) {
        if (waitCount-- == 0U) {
            return;
        }
    }

    ADC_StartCalibration(THERMISTOR_ADC);
    waitCount = ADC_CALIBRATION_WAIT_LIMIT;
    while (ADC_GetCalibrationStatus(THERMISTOR_ADC)) {
        if (waitCount-- == 0U) {
            return;
        }
    }

    g_adcReady = 1;
}

/**
  * @brief  获取当前温度（基于热敏电阻）
  * @param  无
  * @retval 当前温度值（℃）
  */

// 滑动平均滤波缓冲区
typedef struct {
    uint16_t buffer[20];  // 采样缓冲区（20个采样点，增大缓冲区提高稳定性）
    uint8_t index;        // 当前索引
    uint8_t count;        // 采样数量
} FilterBuffer_t;

static FilterBuffer_t g_tempFilter = {0};

// 中值滤波
static uint16_t ApplyMedianFilter(uint16_t *samples, uint8_t count) {
    // 对采样数据进行排序
    for (uint8_t i = 0; i < count - 1; i++) {
        for (uint8_t j = 0; j < count - i - 1; j++) {
            if (samples[j] > samples[j + 1]) {
                uint16_t temp = samples[j];
                samples[j] = samples[j + 1];
                samples[j + 1] = temp;
            }
        }
    }
    
    // 返回中间值
    return samples[count / 2];
}

// 多次采样取平均值（带中值滤波预处理）
static uint8_t ADC_GetFilteredValue(uint16_t *filteredValue) {
    uint16_t samples[20];  // 采样缓冲区
    uint8_t sampleCount = 20;  // 增加采样次数到20次

    if (filteredValue == 0) {
        return 0;
    }
    
    for (uint8_t i = 0; i < sampleCount; i++) {
        uint32_t waitCount = ADC_CONVERSION_WAIT_LIMIT;

        // 配置ADC通道和采样时间（使用最长采样时间提高精度）
        ADC_RegularChannelConfig(THERMISTOR_ADC, THERMISTOR_ADC_CH, 1, ADC_SampleTime_239Cycles5);
        
        ADC_SoftwareStartConvCmd(THERMISTOR_ADC, ENABLE);
        while (!ADC_GetFlagStatus(THERMISTOR_ADC, ADC_FLAG_EOC)) {
            if (waitCount-- == 0U) {
                /* EOC超时沿温度读取失败路径触发加热关断。 */
                return 0;
            }
        }
        
        samples[i] = ADC_GetConversionValue(THERMISTOR_ADC);
    }
    
    // 应用中值滤波去除异常值
    *filteredValue = ApplyMedianFilter(samples, sampleCount);
    return 1;
}

// 滑动平均滤波
static uint16_t ApplyMovingAverage(uint16_t newValue) {
    // 将新值加入缓冲区
    g_tempFilter.buffer[g_tempFilter.index] = newValue;
    g_tempFilter.index = (g_tempFilter.index + 1) % 20;  // 缓冲区大小为20
    
    if (g_tempFilter.count < 20) {
        g_tempFilter.count++;
    }
    
    // 计算平均值
    uint32_t sum = 0;
    for (uint8_t i = 0; i < g_tempFilter.count; i++) {
        sum += g_tempFilter.buffer[i];
    }
    
    return (uint16_t)(sum / g_tempFilter.count);
}
uint8_t TempHeat_GetCurrentTemp(uint8_t *temperature) {
    if (!g_adcReady || temperature == 0) {
        return 0;
    }

    // 1. 多次采样并应用滤波
    uint16_t rawAdc;

    if (!ADC_GetFilteredValue(&rawAdc)) {
        return 0;
    }

    if (rawAdc == 0U || rawAdc >= 4095U) {
        /* ADC轨到轨读数可能表示传感器开路或短路，拒绝参与温控。 */
        return 0;
    }

    uint16_t filteredAdc = ApplyMovingAverage(rawAdc);

    if (filteredAdc == 0U || filteredAdc >= 4095U) {
        return 0;
    }
    
    // 2. 计算热敏电阻阻值
    float voltage = (float)filteredAdc / 4095.0f * 3.3f;  // 假设参考电压为3.3V
    float resistance = THERMISTOR_R_REF * voltage / (3.3f - voltage);
    
    // 3. 使用B值公式计算温度
    float lnR = log(resistance / THERMISTOR_R_REF25);
    float tempK = 1.0f / (1.0f / THERMISTOR_T_REF + lnR / THERMISTOR_B_VALUE);
    float tempC = tempK - 273.15f;
    
    // 4. Reject invalid sensor values instead of clamping them into a valid reading.
    if (!(tempC >= 0.0f && tempC <= 100.0f)) {
        /* 不将无效结果钳位成看似正常的边界温度。 */
        return 0;
    }

    *temperature = (uint8_t)(tempC + 0.5f);
    return 1;
}

static uint8_t ConvertAdcToTemperature(uint16_t rawAdc, uint8_t *temperature) {
    uint16_t filteredAdc;
    float voltage;
    float resistance;
    float lnR;
    float tempK;
    float tempC;

    if (temperature == 0 || rawAdc == 0U || rawAdc >= 4095U) {
        return 0;
    }

    filteredAdc = ApplyMovingAverage(rawAdc);
    if (filteredAdc == 0U || filteredAdc >= 4095U) {
        return 0;
    }

    voltage = (float)filteredAdc / 4095.0f * 3.3f;
    resistance = THERMISTOR_R_REF * voltage / (3.3f - voltage);
    lnR = log(resistance / THERMISTOR_R_REF25);
    tempK = 1.0f / (1.0f / THERMISTOR_T_REF + lnR / THERMISTOR_B_VALUE);
    tempC = tempK - 273.15f;

    if (!(tempC >= 0.0f && tempC <= 100.0f)) {
        return 0;
    }

    *temperature = (uint8_t)(tempC + 0.5f);
    return 1;
}

TempHeatSampleStatus_t TempHeat_TryGetCurrentTemp(uint8_t *temperature) {
    uint16_t medianAdc;

    if (!g_adcReady || temperature == 0) {
        return TEMPHEAT_SAMPLE_ERROR;
    }

    if (!g_asyncSampling) {
        ADC_RegularChannelConfig(THERMISTOR_ADC, THERMISTOR_ADC_CH, 1, ADC_SampleTime_239Cycles5);
        ADC_ClearFlag(THERMISTOR_ADC, ADC_FLAG_EOC);
        ADC_SoftwareStartConvCmd(THERMISTOR_ADC, ENABLE);
        g_asyncSampleIndex = 0;
        g_asyncWaitCount = 0;
        g_asyncSampling = 1;
        return TEMPHEAT_SAMPLE_PENDING;
    }

    if (ADC_GetFlagStatus(THERMISTOR_ADC, ADC_FLAG_EOC) == RESET) {
        if (++g_asyncWaitCount >= ADC_ASYNC_WAIT_LIMIT) {
            g_asyncSampling = 0;
            g_asyncSampleIndex = 0;
            return TEMPHEAT_SAMPLE_ERROR;
        }
        return TEMPHEAT_SAMPLE_PENDING;
    }

    g_asyncSamples[g_asyncSampleIndex++] = ADC_GetConversionValue(THERMISTOR_ADC);
    g_asyncWaitCount = 0;
    if (g_asyncSampleIndex < 20U) {
        ADC_SoftwareStartConvCmd(THERMISTOR_ADC, ENABLE);
        return TEMPHEAT_SAMPLE_PENDING;
    }

    g_asyncSampling = 0;
    g_asyncSampleIndex = 0;
    medianAdc = ApplyMedianFilter(g_asyncSamples, 20U);
    if (!ConvertAdcToTemperature(medianAdc, temperature)) {
        return TEMPHEAT_SAMPLE_ERROR;
    }

    return TEMPHEAT_SAMPLE_READY;
}

/**
  * @brief  设置加热功率（继电器控制）
  * @param  power: 功率值（0-100），0表示关闭，>0表示开启
  * @retval 无
  */
void TempHeat_SetPower(uint8_t power) {
    if (power > 0) {
        // 继电器吸合，开启加热
        GPIO_SetBits(HEAT_PORT, HEAT_PIN);
    } else {
        // 继电器断开，关闭加热
        GPIO_ResetBits(HEAT_PORT, HEAT_PIN);
    }
}
