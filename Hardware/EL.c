#include "EL.h"

/**
  * @brief  使能模块初始化
  * @param  无
  * @retval 无
  */
void EL_Init(void) {
    RCC_APB2PeriphClockCmd(EL_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = EL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(EL_PORT, &GPIO_InitStructure);
    
    // 默认禁用系统
    EL_Disable();
}

/**
  * @brief  启用系统
  * @param  无
  * @retval 无
  */
void EL_Enable(void) {
    GPIO_SetBits(EL_PORT, EL_PIN);
}

/**
  * @brief  禁用系统
  * @param  无
  * @retval 无
  */
void EL_Disable(void) {
    GPIO_ResetBits(EL_PORT, EL_PIN);
}
