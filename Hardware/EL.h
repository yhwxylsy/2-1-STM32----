#ifndef __EL_H
#define __EL_H

#include "stm32f10x.h"

// 使能控制端口定义
#define EL_PORT    GPIOB
#define EL_PIN     GPIO_Pin_4
#define EL_RCC     RCC_APB2Periph_GPIOB

// 函数声明
void EL_Init(void);
void EL_Enable(void);
void EL_Disable(void);

#endif
