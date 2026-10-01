#include "Pressure.h"
#include "system_stm32f10x.h"

static uint8_t g_hx711Waiting;
static uint16_t g_hx711StartCount;

/**
  * @brief  延时微秒
  * @param  us: 微秒数
  * @retval 无
  */
static void Delay_us(uint32_t us) {
    us *= (SystemCoreClock / 1000000) / 8;
    while (us--);
}

/**
  * @brief  HX711压力传感器初始化
  * @param  无
  * @retval 无
  */
void Pressure_Init(void) {
    RCC_APB2PeriphClockCmd(HX711_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 初始化SCK引脚为推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = HX711_SCK_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HX711_SCK_PORT, &GPIO_InitStructure);
    
    // 初始化DT引脚为上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = HX711_DT_PIN;
    GPIO_Init(HX711_DT_PORT, &GPIO_InitStructure);
    
    // 初始状态SCK为低电平
    GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
}

/**
  * @brief  读取HX711数据
  * @param  无
  * @retval 压力传感器原始数据
  */
uint8_t HX711_Read(int32_t *weight) {
    int32_t data = 0;
    uint8_t i = 0;
    uint16_t startCount;
    uint16_t currentCount;
    uint32_t elapsedTicks;
    uint32_t timerPeriod;

    if (weight == 0 || (TIM2->CR1 & TIM_CR1_CEN) == 0) {
        return 0;
    }

    /* 用TIM2计数差限制等待时间，避免HX711未就绪时卡住主循环。 */
    startCount = TIM_GetCounter(TIM2);
    timerPeriod = (uint32_t)TIM2->ARR + 1U;
    while (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
        currentCount = TIM_GetCounter(TIM2);
        if (currentCount >= startCount) {
            elapsedTicks = currentCount - startCount;
        } else {
            elapsedTicks = timerPeriod - startCount + currentCount;
        }
        if (elapsedTicks >= HX711_READY_TIMEOUT_TICKS) {
            return 0;
        }
    }
    
    // 读取24位数据
    for (i = 0; i < 24; i++) {
        // 发送时钟脉冲
        GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
        
        // 读取数据位
        data <<= 1;
        if (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
            data++;
        }
        
        GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
    }
    
    // 发送第25个时钟脉冲，选择下一次采样的增益
    // 25个脉冲：通道A，增益128
    GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);
    GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);
    
    // 将24位有符号数扩展为32位有符号数
    // 如果最高位(第24位)为1，则符号扩展
    if (data & 0x800000) {
        data |= 0xFF000000;  // 符号扩展到32位
    }
    
    *weight = data;
    return 1;
}

static uint8_t HX711_ReadReady(int32_t *weight) {
    int32_t data = 0;
    uint8_t i;

    if (weight == 0 || GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
        return 0;
    }

    for (i = 0; i < 24; i++) {
        GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
        data <<= 1;
        if (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
            data++;
        }
        GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
    }

    GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);
    GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);

    if (data & 0x800000) {
        data |= 0xFF000000;
    }

    *weight = data;
    return 1;
}

PressureReadStatus_t Pressure_TryDetect(uint8_t *pressureDetected) {
    int32_t weight;
    uint16_t currentCount;
    uint32_t elapsedTicks;
    uint32_t timerPeriod;

    if (pressureDetected == 0 || (TIM2->CR1 & TIM_CR1_CEN) == 0) {
        return PRESSURE_READ_TIMEOUT;
    }

    if (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
        if (!g_hx711Waiting) {
            g_hx711Waiting = 1;
            g_hx711StartCount = TIM_GetCounter(TIM2);
        }

        currentCount = TIM_GetCounter(TIM2);
        timerPeriod = (uint32_t)TIM2->ARR + 1U;
        if (currentCount >= g_hx711StartCount) {
            elapsedTicks = currentCount - g_hx711StartCount;
        } else {
            elapsedTicks = timerPeriod - g_hx711StartCount + currentCount;
        }

        if (elapsedTicks >= HX711_READY_TIMEOUT_TICKS) {
            g_hx711Waiting = 0;
            return PRESSURE_READ_TIMEOUT;
        }
        return PRESSURE_READ_PENDING;
    }

    g_hx711Waiting = 0;
    if (!HX711_ReadReady(&weight)) {
        return PRESSURE_READ_TIMEOUT;
    }

    *pressureDetected = (weight > HX711_THRESHOLD) ? 1U : 0U;
    return PRESSURE_READ_READY;
}

/**
  * @brief  检测压力状态
  * @param  无
  * @retval 0：无压力，1：有压力
  */
uint8_t Pressure_Detect(uint8_t *pressureDetected) {
    // 读取HX711数据
    int32_t weight;

    /* 读取失败与有效读数中的“无压力”必须区分。 */
    if (pressureDetected == 0 || !HX711_Read(&weight)) {
        return 0;
    }
    
    // 超过阈值表示有压力
    // 注意：实际使用时需要校准传感器，确定合适的阈值
    *pressureDetected = (weight > HX711_THRESHOLD) ? 1 : 0;
    return 1;
}
