#include "Level.h"

/**
  * @brief  反射式红外传感器初始化
  * @param  无
  * @retval 无
  */
void Level_Init(void) {
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(LEVEL_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 初始化数字输出引脚为上拉输入（连接传感器DQ引脚）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = LEVEL_DQ_PIN;
    GPIO_Init(LEVEL_DQ_PORT, &GPIO_InitStructure);
}

/**
  * @brief  检测液面状态
  * @param  无
  * @retval 0：无液体（液面低于传感器），1：有液体（液面高于传感器）
  */
uint8_t Level_Detect(void) {
    // 读取传感器数字输出状态（DQ引脚）
    uint8_t recvState = GPIO_ReadInputDataBit(LEVEL_DQ_PORT, LEVEL_DQ_PIN);
    
    // 反射式红外传感器逻辑（带DQ数字输出）：
    // 当有液体时，红外光被液体表面反射，传感器内部电路检测到反射光，DQ引脚输出低电平(0)
    // 当无液体时，红外光穿透容器或被容器底部吸收，传感器内部电路未检测到反射光，DQ引脚输出高电平(1)
    if (recvState == 0) {
        return 1;  // 有液体（接收到反射光）
    } else {
        return 0;  // 无液体（未接收到反射光）
    }
}
