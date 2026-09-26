#include "Pressure.h"
#include "system_stm32f10x.h"

/**
  * @brief  延时微秒
  * @param  us: 微秒数
  * @retval 无
  */
static void Delay_us(uint32_t us) {
    us *= (SystemCoreClock / 1000000) / 8;
    while (us--);
}

/**
  * @brief  HX711压力传感器初始化
  * @param  无
  * @retval 无
  */
void Pressure_Init(void) {
    RCC_APB2PeriphClockCmd(HX711_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 初始化SCK引脚为推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = HX711_SCK_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HX711_SCK_PORT, &GPIO_InitStructure);
    
    // 初始化DT引脚为上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = HX711_DT_PIN;
    GPIO_Init(HX711_DT_PORT, &GPIO_InitStructure);
    
    // 初始状态SCK为低电平
    GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
}

/**
  * @brief  读取HX711数据
  * @param  无
  * @retval 压力传感器原始数据
  */
int32_t HX711_Read(void) {
    int32_t data = 0;
    uint8_t i = 0;
    
    // 等待DT引脚变低，表示数据准备就绪
    while (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN));
    
    // 读取24位数据
    for (i = 0; i < 24; i++) {
        // 发送时钟脉冲
        GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
        
        // 读取数据位
        data <<= 1;
        if (GPIO_ReadInputDataBit(HX711_DT_PORT, HX711_DT_PIN)) {
            data++;
        }
        
        GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
        Delay_us(1);
    }
    
    // 发送第25个时钟脉冲，选择下一次采样的增益
    // 25个脉冲：通道A，增益128
    GPIO_SetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);
    GPIO_ResetBits(HX711_SCK_PORT, HX711_SCK_PIN);
    Delay_us(1);
    
    // 将24位有符号数扩展为32位有符号数
    // 如果最高位(第24位)为1，则符号扩展
    if (data & 0x800000) {
        data |= 0xFF000000;  // 符号扩展到32位
    }
    
    return data;
}

/**
  * @brief  检测压力状态
  * @param  无
  * @retval 0：无压力，1：有压力
  */
uint8_t Pressure_Detect(void) {
    // 读取HX711数据
    int32_t weight = HX711_Read();
    
    // 超过阈值表示有压力
    // 注意：实际使用时需要校准传感器，确定合适的阈值
    return (weight > HX711_THRESHOLD) ? 1 : 0;
}
