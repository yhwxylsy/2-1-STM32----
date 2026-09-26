#ifndef __LEVEL_H
#define __LEVEL_H

#include "stm32f10x.h"

// 反射式红外传感器端口定义
// 传感器引脚说明：
// VCC - 接电源正极 (3.3V/5V)
// GND - 接电源负极
// AQ  - 模拟输出 (可接ADC引脚，用于精确测量反射强度)
// DQ  - 数字输出 (接GPIO输入，用于简单的有无检测)
#define LEVEL_DQ_PORT     GPIOB  // 数字输出端口 (连接传感器DQ引脚)
#define LEVEL_DQ_PIN      GPIO_Pin_12  // 数字输出引脚
#define LEVEL_AQ_PORT     GPIOB  // 模拟输出端口 (连接传感器AQ引脚，可选使用)
#define LEVEL_AQ_PIN      GPIO_Pin_13  // 模拟输出引脚
#define LEVEL_RCC         RCC_APB2Periph_GPIOB  // 时钟使能

// 函数声明
void Level_Init(void);
uint8_t Level_Detect(void);

#endif
