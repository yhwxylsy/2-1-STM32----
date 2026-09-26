#ifndef __CONTROL_H
#define __CONTROL_H

#include "stm32f10x.h"

// 系统状态枚举
typedef enum {
    SYSTEM_OFF,      // 系统关闭
    SYSTEM_STANDBY,  // 待机状态（水瓶在加热板上但未达到设定温度）
    SYSTEM_HEATING,  // 加热状态
    SYSTEM_KEEPING,  // 保温状态
    SYSTEM_ERROR     // 错误状态
} SystemState_t;

// 系统配置结构体
typedef struct {
    uint8_t targetTemp;        // 目标温度（℃）
    uint8_t currentTemp;       // 当前温度（℃）
    uint8_t pressureDetected;  // 压力检测状态（0：无压力，1：有压力）
    uint8_t liquidLevel;       // 液面检测状态（0：无液体，1：有液体）
    SystemState_t systemState; // 系统状态
} SystemConfig_t;

// 函数声明
void Control_Init(void);
void Control_Update(void);
void Control_SetTargetTemp(uint8_t temp);
uint8_t Control_GetTargetTemp(void);
uint8_t Control_GetCurrentTemp(void);
SystemState_t Control_GetSystemState(void);

#endif
