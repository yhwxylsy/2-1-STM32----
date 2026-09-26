#include "Serial.h"
#include "math.h"

// 全局变量定义
uint8_t Serial_RxData = 0;
uint8_t Serial_RxFlag = 0;

/**
  * @brief  串口初始化函数
  * @param  无
  * @retval 无
  */
void Serial_Init(void) {
    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(SERIAL_USART_RCC | SERIAL_GPIO_RCC, ENABLE);
    
    // 2. 配置GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置发送引脚 (PA9) 为复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = SERIAL_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERIAL_TX_PORT, &GPIO_InitStructure);
    
    // 配置接收引脚 (PA10) 为浮空输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = SERIAL_RX_PIN;
    GPIO_Init(SERIAL_RX_PORT, &GPIO_InitStructure);
    
    // 3. 配置USART
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = SERIAL_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;  // 发送和接收模式
    USART_Init(SERIAL_USART, &USART_InitStructure);
    
    // 4. 配置中断控制器
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 5. 使能USART接收中断
    USART_ITConfig(SERIAL_USART, USART_IT_RXNE, ENABLE);
    
    // 6. 使能USART
    USART_Cmd(SERIAL_USART, ENABLE);
}

/**
  * @brief  发送单个字节
  * @param  byte: 要发送的字节
  * @retval 无
  */
void Serial_SendByte(uint8_t byte) {
    // 等待发送缓冲区为空
    while (USART_GetFlagStatus(SERIAL_USART, USART_FLAG_TXE) == RESET);
    
    // 发送数据
    USART_SendData(SERIAL_USART, byte);
}

/**
  * @brief  发送字符串
  * @param  str: 要发送的字符串指针
  * @retval 无
  */
void Serial_SendString(uint8_t* str) {
    while (*str != '\0') {
        Serial_SendByte(*str);
        str++;
    }
}

/**
  * @brief  发送数字
  * @param  num: 要发送的数字
  * @param  len: 数字的位数
  * @retval 无
  */
void Serial_SendNumber(uint32_t num, uint8_t len) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        // 从高位到低位依次发送
        Serial_SendByte((num / (uint32_t)pow(10, len - i - 1)) % 10 + '0');
    }
}

/**
  * @brief  接收单个字节
  * @param  无
  * @retval 接收到的字节
  */
uint8_t Serial_ReceiveByte(void) {
    // 清除接收标志
    Serial_RxFlag = 0;
    
    // 返回接收到的数据
    return Serial_RxData;
}

/**
  * @brief  获取接收标志
  * @param  无
  * @retval 接收标志（1: 接收到数据, 0: 未接收到数据）
  */
uint8_t Serial_GetRxFlag(void) {
    return Serial_RxFlag;
}
