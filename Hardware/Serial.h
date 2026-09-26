#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

/* 串口参数配置 */
#define SERIAL_USART            USART1
#define SERIAL_USART_RCC        RCC_APB2Periph_USART1
#define SERIAL_GPIO_RCC         RCC_APB2Periph_GPIOA
#define SERIAL_TX_PORT          GPIOA
#define SERIAL_TX_PIN           GPIO_Pin_9
#define SERIAL_RX_PORT          GPIOA
#define SERIAL_RX_PIN           GPIO_Pin_10

/* 波特率设置（可根据需要修改） */
#define SERIAL_BAUDRATE         9600
#define SERIAL_RX_BUFFER_SIZE   128U /* 必须为2的幂，索引才能用位掩码回绕。 */
#define SERIAL_TX_BUFFER_SIZE   256U /* 必须为2的幂，索引才能自然回绕。 */

/* 函数声明 */
void Serial_Init(void);
/* 仅将字节放入队列；实际发送由USART1 TXE中断完成。 */
void Serial_SendByte(uint8_t byte);
void Serial_SendString(uint8_t* str);
void Serial_SendNumber(uint32_t num, uint8_t len);
uint8_t Serial_ReceiveByte(void);
uint8_t Serial_GetRxFlag(void);
/* 由USART1 TXE中断调用，用于发送队列中的下一个字节。 */
void Serial_TxEmptyISR(void);
/* 由接收中断调用；队列满时丢弃新字节并累计溢出。 */
void Serial_RxPush(uint8_t byte);
/* 获取接收队列满时累计丢弃的字节数。 */
uint32_t Serial_GetRxOverflowCount(void);
/* 获取发送队列满时累计丢弃的字节数。 */
uint32_t Serial_GetTxOverflowCount(void);

#endif
