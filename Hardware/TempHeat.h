#ifndef __TEMPHEAT_H
#define __TEMPHEAT_H

#include "stm32f10x.h"
#include <math.h>

// 热敏电阻传感器端口定义 (AQ为模拟输出引脚)
#define THERMISTOR_PORT    GPIOA
#define THERMISTOR_PIN     GPIO_Pin_0
#define THERMISTOR_RCC     RCC_APB2Periph_GPIOA
#define THERMISTOR_ADC     ADC1
#define THERMISTOR_ADC_CH  ADC_Channel_0

// 加热控制端口定义
#define HEAT_PORT    GPIOA
#define HEAT_PIN     GPIO_Pin_1
#define HEAT_RCC     RCC_APB2Periph_GPIOA
#define HEAT_TIM     TIM3
#define HEAT_TIM_CH  TIM_Channel_2
#define HEAT_TIM_RCC RCC_APB1Periph_TIM3

// 热敏电阻参数（需要根据实际传感器校准）
#define THERMISTOR_R_REF    10000  // 参考电阻值（10KΩ）
#define THERMISTOR_B_VALUE  3950   // B值（3950K）
#define THERMISTOR_T_REF    298.15 // 参考温度（25℃，单位：开尔文）
#define THERMISTOR_R_REF25  10000  // 25℃时的电阻值（10KΩ）

// 函数声明
void TempHeat_Init(void);
typedef enum {
	TEMPHEAT_SAMPLE_PENDING,
	TEMPHEAT_SAMPLE_READY,
	TEMPHEAT_SAMPLE_ERROR
} TempHeatSampleStatus_t;
/* 返回1表示温度有效；成功时通过参数返回摄氏温度。 */
uint8_t TempHeat_GetCurrentTemp(uint8_t *temperature);
/* 非阻塞推进一次ADC采样；20个样本跨多个调度周期完成。 */
TempHeatSampleStatus_t TempHeat_TryGetCurrentTemp(uint8_t *temperature);
void TempHeat_SetPower(uint8_t power);

#endif
