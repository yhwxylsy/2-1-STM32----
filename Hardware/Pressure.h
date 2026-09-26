#ifndef __PRESSURE_H
#define __PRESSURE_H

#include "stm32f10x.h"

// HX711压力传感器端口定义
#define HX711_SCK_PORT    GPIOB
#define HX711_SCK_PIN     GPIO_Pin_6
#define HX711_DT_PORT     GPIOB
#define HX711_DT_PIN      GPIO_Pin_7
#define HX711_RCC         RCC_APB2Periph_GPIOB
#define HX711_READY_TIMEOUT_TICKS 1800U

// 校准参数
#define HX711_THRESHOLD   1000  // 压力检测阈值，需根据实际传感器校准

// 函数声明
void Pressure_Init(void);
uint8_t Pressure_Detect(uint8_t *pressureDetected);
uint8_t HX711_Read(int32_t *weight);

#endif
