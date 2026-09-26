#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

// 串口参数配置
#define SERIAL_USART            USART1
#define SERIAL_USART_RCC        RCC_APB2Periph_USART1
#define SERIAL_GPIO_RCC         RCC_APB2Periph_GPIOA
#define SERIAL_TX_PORT          GPIOA
#define SERIAL_TX_PIN           GPIO_Pin_9
#define SERIAL_RX_PORT          GPIOA
#define SERIAL_RX_PIN           GPIO_Pin_10

// 波特率设置（可根据需要修改）
#define SERIAL_BAUDRATE         9600

// 函数声明
void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendString(uint8_t* str);
void Serial_SendNumber(uint32_t num, uint8_t len);
uint8_t Serial_ReceiveByte(void);
uint8_t Serial_GetRxFlag(void);

// 全局变量声明
extern uint8_t Serial_RxData;
extern uint8_t Serial_RxFlag;

#endif
