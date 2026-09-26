#include "Serial.h"

/* ISR单生产者、主循环单消费者；volatile保证共享索引每次都从内存读取。 */
static volatile uint8_t g_rxBuffer[SERIAL_RX_BUFFER_SIZE];
static volatile uint8_t g_rxHead = 0;
static volatile uint8_t g_rxTail = 0;
static volatile uint32_t g_rxOverflowCount = 0;
/* 主循环写入发送队列，USART1 TXE中断负责消费。 */
static volatile uint8_t g_txBuffer[SERIAL_TX_BUFFER_SIZE];
static volatile uint8_t g_txHead = 0;
static volatile uint8_t g_txTail = 0;
static volatile uint32_t g_txOverflowCount = 0;

/**
  * @brief  串口初始化函数
  * @param  无
  * @retval 无
  */
void Serial_Init(void) {
    /* 1. 使能时钟 */
    RCC_APB2PeriphClockCmd(SERIAL_USART_RCC | SERIAL_GPIO_RCC, ENABLE);
    
    /* 2. 配置GPIO */
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 配置发送引脚 (PA9) 为复用推挽输出 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = SERIAL_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERIAL_TX_PORT, &GPIO_InitStructure);
    
    /* 配置接收引脚 (PA10) 为浮空输入 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = SERIAL_RX_PIN;
    GPIO_Init(SERIAL_RX_PORT, &GPIO_InitStructure);
    
    /* 3. 配置USART */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = SERIAL_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;  /* 发送和接收模式 */
    USART_Init(SERIAL_USART, &USART_InitStructure);
    
    /* 4. 配置中断控制器 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /* 5. 使能USART接收中断 */
    USART_ITConfig(SERIAL_USART, USART_IT_TXE, DISABLE);
    USART_ITConfig(SERIAL_USART, USART_IT_RXNE, ENABLE);
    
    /* 6. 使能USART */
    USART_Cmd(SERIAL_USART, ENABLE);
}

/**
  * @brief  发送单个字节
  * @param  byte: 要发送的字节
  * @retval 无
  */
void Serial_SendByte(uint8_t byte) {
  uint8_t head = g_txHead;
  uint8_t next = (uint8_t)(head + 1U);

  if (next == g_txTail) {
    /* 队列满時丢弃新字节，避免发送接口阻塞主循环。 */
    g_txOverflowCount++;
    return;
  }

  g_txBuffer[head] = byte;
  g_txHead = next;
  USART_ITConfig(SERIAL_USART, USART_IT_TXE, ENABLE);
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
  uint32_t divisor = 1U;
  uint8_t i;

  /* 只输出指定宽度的低位十进制数字，位权通过整数运算递减。 */
  /* uint32_t最多10位，限制位宽以避免十进制除数溢出。 */
  if (len > 10U) {
    len = 10U;
  }

  for (i = 1U; i < len; i++) {
    divisor *= 10U;
  }

  for (i = 0U; i < len; i++) {
    Serial_SendByte((uint8_t)((num / divisor) % 10U) + '0');
    divisor /= 10U;
    }
}

/**
  * @brief  接收单个字节
  * @param  无
  * @retval 接收到的字节
  */
uint8_t Serial_ReceiveByte(void) {
  uint8_t tail = g_rxTail;
  uint8_t byte;

  if (tail == g_rxHead) {
    /* 空队列时返回0；调用方应先检查Serial_GetRxFlag()。 */
    return 0;
  }

  byte = g_rxBuffer[tail];
  g_rxTail = (tail + 1U) & (SERIAL_RX_BUFFER_SIZE - 1U);
  return byte;
}

/**
  * @brief  获取接收标志
  * @param  无
  * @retval 接收标志（1: 接收到数据, 0: 未接收到数据）
  */
uint8_t Serial_GetRxFlag(void) {
  return g_rxHead != g_rxTail;
}

void Serial_RxPush(uint8_t byte) {
  uint8_t head = g_rxHead;
  uint8_t next = (head + 1U) & (SERIAL_RX_BUFFER_SIZE - 1U);

  if (next == g_rxTail) {
    /* 队列满时丢弃新数据，保留尚未消费的旧数据并记录溢出。 */
    g_rxOverflowCount++;
    return;
  }

  g_rxBuffer[head] = byte;
  g_rxHead = next;
}

uint32_t Serial_GetRxOverflowCount(void) {
  return g_rxOverflowCount;
}

void Serial_TxEmptyISR(void) {
  uint8_t tail = g_txTail;

  if (tail == g_txHead) {
    USART_ITConfig(SERIAL_USART, USART_IT_TXE, DISABLE);
    return;
  }

  USART_SendData(SERIAL_USART, g_txBuffer[tail]);
  g_txTail = (uint8_t)(tail + 1U);
}

uint32_t Serial_GetTxOverflowCount(void) {
  return g_txOverflowCount;
}
