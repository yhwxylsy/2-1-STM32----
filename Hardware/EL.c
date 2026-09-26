#include "EL.h"

/**
  * @brief  使能模块初始化
  * @param  无
  * @retval 无
  */
void EL_Init(void) {
  RCC_APB2PeriphClockCmd(EL_RCC | RCC_APB2Periph_AFIO, ENABLE);
  /* PB4复用为普通GPIO；关闭JTAG-DP但保留SW-DP调试接口。 */
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
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
