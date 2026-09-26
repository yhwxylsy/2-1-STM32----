#include "Display.h"
#include "Control.h"

extern volatile uint32_t timer_seconds;

// 按键状态变量
static uint8_t lastUpState = 1;
static uint8_t lastDownState = 1;
static uint32_t debounceTime = 0;

/**
  * @brief  OLED显示和按键初始化
  * @param  无
  * @retval 无
  */
void Display_Init(void) {
    // 初始化OLED
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(1, 1, "Water Heater");
    OLED_ShowString(2, 1, "Target: --C");
    OLED_ShowString(3, 1, "Current: --C");
    OLED_ShowString(4, 1, "Status: OFF");
    
    // 初始化按键
    RCC_APB2PeriphClockCmd(BTN_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = BTN_UP_PIN | BTN_DOWN_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BTN_UP_PORT, &GPIO_InitStructure);
}

/**
  * @brief  更新OLED显示内容
  * @param  targetTemp: 目标温度
  * @param  currentTemp: 当前温度
  * @param  state: 系统状态
  * @retval 无
  */
void Display_Update(uint8_t targetTemp, uint8_t currentTemp, SystemState_t state) {
    // 显示目标温度
    OLED_ShowNum(2, 9, targetTemp, 2);
    
    // 显示当前温度
    OLED_ShowNum(3, 10, currentTemp, 2);
    
    // 显示系统状态
    OLED_SetCursor(4, 8);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(0x20);  // 显示空格，确保覆盖之前的字符
    }
    
    switch (state) {
        case SYSTEM_OFF:
            OLED_ShowString(4, 8, "OFF");
            break;
        case SYSTEM_STANDBY:
            OLED_ShowString(4, 8, "STANDBY");
            break;
        case SYSTEM_HEATING:
            OLED_ShowString(4, 8, "HEATING");
            break;
        case SYSTEM_KEEPING:
            OLED_ShowString(4, 8, "KEEPING");
            break;
        case SYSTEM_ERROR:
            OLED_ShowString(4, 8, "ERROR");
            break;
    }
}

/**
  * @brief  检查按键状态并处理
  * @param  无
  * @retval 无
  */
void Display_CheckButtons(void) {
    // 简单的按键消抖
    if (GetTick() - debounceTime < 100) return;
    
    // 读取按键状态
    uint8_t upState = GPIO_ReadInputDataBit(BTN_UP_PORT, BTN_UP_PIN);
    uint8_t downState = GPIO_ReadInputDataBit(BTN_DOWN_PORT, BTN_DOWN_PIN);
    
    // 处理向上按键
    if (lastUpState && !upState) {
        uint8_t currentTemp = Control_GetTargetTemp();
        if (currentTemp < 100) {
            Control_SetTargetTemp(currentTemp + 5);  // 每次增加5℃
        }
        debounceTime = GetTick();
    }
    
    // 处理向下按键
    if (lastDownState && !downState) {
        uint8_t currentTemp = Control_GetTargetTemp();
        if (currentTemp > 30) {
            Control_SetTargetTemp(currentTemp - 5);  // 每次减少5℃
        }
        debounceTime = GetTick();
    }
    
    // 更新按键状态
    lastUpState = upState;
    lastDownState = downState;
}

/**
  * @brief  获取系统运行时间（毫秒）
  * @param  无
  * @retval 系统运行时间
  */
uint32_t GetTick(void) {
    uint32_t seconds;
    uint32_t timerPeriod;
    uint16_t counter;
    uint32_t primask = __get_PRIMASK();

    /* 原子读取秒计数与CNT，处理恰好跨越TIM2更新事件的情况。 */
    __disable_irq();
    seconds = timer_seconds;
    counter = (uint16_t)TIM2->CNT;
    if ((TIM2->SR & TIM_SR_UIF) != 0) {
        seconds++;
        counter = (uint16_t)TIM2->CNT;
    }
    if ((primask & 1U) == 0) {
        __enable_irq();
    }

    timerPeriod = (uint32_t)TIM2->ARR + 1U;
    return seconds * 1000U + ((uint32_t)counter * 1000U) / timerPeriod;
}
