#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "stm32f10x.h"
#include "Control.h"
#include "OLED.h"

// 按键端口定义
#define BTN_UP_PORT    GPIOA
#define BTN_UP_PIN     GPIO_Pin_2
#define BTN_DOWN_PORT  GPIOA
#define BTN_DOWN_PIN   GPIO_Pin_3
#define BTN_RCC        RCC_APB2Periph_GPIOA

// 函数声明
void Display_Init(void);
void Display_Update(uint8_t targetTemp, uint8_t currentTemp, SystemState_t state);
void Display_CheckButtons(void);
uint32_t GetTick(void);

#endif
