#include "Control.h"
#include "TempHeat.h"
#include "Pressure.h"
#include "Level.h"
#include "Display.h"
#include "EL.h"
#include "Serial.h"

// 系统配置全局变量
static SystemConfig_t g_systemConfig;
static SystemFault_t g_systemFault;

/**
  * @brief  主控模块初始化
  * @param  无
  * @retval 无
  */
void Control_Init(void) {
    // 初始化默认配置
    g_systemConfig.targetTemp = 50;  // 默认目标温度50℃
    g_systemConfig.currentTemp = 0;
    g_systemConfig.pressureDetected = 0;
    g_systemConfig.liquidLevel = 0;
    g_systemConfig.systemState = SYSTEM_OFF;
    g_systemFault = SYSTEM_FAULT_NONE;
}

/**
  * @brief  更新系统状态
  * @param  无
  * @retval 无
  */
void Control_Update(void) {
    uint8_t pressureDetected;
    uint8_t currentTemp;

    // 读取传感器数据
    if (!TempHeat_GetCurrentTemp(&currentTemp)) {
        /* 温度数据无效时锁定错误状态并立即撤销加热。 */
        g_systemConfig.systemState = SYSTEM_ERROR;
        g_systemFault = SYSTEM_FAULT_TEMPERATURE;
        TempHeat_SetPower(0);
        EL_Disable();
        Display_Update(g_systemConfig.targetTemp, g_systemConfig.currentTemp, g_systemConfig.systemState);
        return;
    }
    g_systemConfig.currentTemp = currentTemp;

    if (!Pressure_Detect(&pressureDetected)) {
        /* 压力传感器读取失败不能等同于正常的无压力读数。 */
        g_systemConfig.pressureDetected = 0;
        g_systemConfig.systemState = SYSTEM_ERROR;
        g_systemFault = SYSTEM_FAULT_PRESSURE;
        TempHeat_SetPower(0);
        EL_Disable();
        Display_Update(g_systemConfig.targetTemp, g_systemConfig.currentTemp, g_systemConfig.systemState);
        return;
    }
    g_systemConfig.pressureDetected = pressureDetected;
    g_systemConfig.liquidLevel = Level_Detect();
    
    // 根据系统状态进行控制
    switch (g_systemConfig.systemState) {
        case SYSTEM_OFF:
            // 检查是否有水瓶放置
            if (g_systemConfig.pressureDetected) {
                g_systemConfig.systemState = SYSTEM_STANDBY;
                EL_Enable();  // 启用系统
            }
            break;
            
        case SYSTEM_STANDBY:
            // 检查是否有液体
            if (!g_systemConfig.pressureDetected) {
                g_systemConfig.systemState = SYSTEM_OFF;
                g_systemFault = SYSTEM_FAULT_NONE;
                EL_Disable();  // 禁用系统
            } else if (g_systemConfig.liquidLevel) {
                g_systemConfig.systemState = SYSTEM_HEATING;
            }
            break;
            
        case SYSTEM_HEATING:
            // 检查安全条件
            if (!g_systemConfig.pressureDetected) {
                g_systemConfig.systemState = SYSTEM_OFF;
                g_systemFault = SYSTEM_FAULT_NONE;
                TempHeat_SetPower(0);
                EL_Disable();
            } else if (!g_systemConfig.liquidLevel) {
                g_systemConfig.systemState = SYSTEM_ERROR;
                g_systemFault = SYSTEM_FAULT_LIQUID_LEVEL;
                TempHeat_SetPower(0);
                /* 同时撤销模块使能，避免故障后外围仍保持工作。 */
                EL_Disable();
            } else {
                // 温度控制
                if (g_systemConfig.currentTemp < g_systemConfig.targetTemp) {
                    // 根据温度差调整加热功率
                    uint8_t power = 50 + (g_systemConfig.targetTemp - g_systemConfig.currentTemp) * 5;
                    if (power > 100) power = 100;
                    TempHeat_SetPower(power);
                } else {
                    g_systemConfig.systemState = SYSTEM_KEEPING;
                    TempHeat_SetPower(20);  // 保温功率
                }
            }
            break;
            
        case SYSTEM_KEEPING:
            // 检查安全条件
            if (!g_systemConfig.pressureDetected) {
                g_systemConfig.systemState = SYSTEM_OFF;
                g_systemFault = SYSTEM_FAULT_NONE;
                TempHeat_SetPower(0);
                EL_Disable();
            } else if (!g_systemConfig.liquidLevel) {
                g_systemConfig.systemState = SYSTEM_ERROR;
                g_systemFault = SYSTEM_FAULT_LIQUID_LEVEL;
                TempHeat_SetPower(0);
                /* 同时撤销模块使能，避免故障后外围仍保持工作。 */
                EL_Disable();
            } else {
                // 保温控制
                if (g_systemConfig.currentTemp < g_systemConfig.targetTemp - 2) {
                    g_systemConfig.systemState = SYSTEM_HEATING;
                } else if (g_systemConfig.currentTemp > g_systemConfig.targetTemp + 2) {
                    TempHeat_SetPower(0);
                } else {
                    TempHeat_SetPower(20);  // 维持保温功率
                }
            }
            break;
            
        case SYSTEM_ERROR:
            // 错误状态处理
            if (!g_systemConfig.pressureDetected) {
                g_systemConfig.systemState = SYSTEM_OFF;
                g_systemFault = SYSTEM_FAULT_NONE;
                EL_Disable();
            }
            break;
    }
    
    // 更新显示
    Display_Update(g_systemConfig.targetTemp, g_systemConfig.currentTemp, g_systemConfig.systemState);
}

/**
  * @brief  设置目标温度
  * @param  temp: 目标温度值（℃）
  * @retval 无
  */
void Control_SetTargetTemp(uint8_t temp) {
    if (temp >= 30 && temp <= 100) {  // 限制温度范围30-100℃
        g_systemConfig.targetTemp = temp;
    }
}

/**
  * @brief  获取目标温度
  * @param  无
  * @retval 目标温度值（℃）
  */
uint8_t Control_GetTargetTemp(void) {
    return g_systemConfig.targetTemp;
}

/**
  * @brief  获取当前温度
  * @param  无
  * @retval 当前温度值（℃）
  */
uint8_t Control_GetCurrentTemp(void) {
    return g_systemConfig.currentTemp;
}

/**
  * @brief  获取系统状态
  * @param  无
  * @retval 系统状态
  */
SystemState_t Control_GetSystemState(void) {
    return g_systemConfig.systemState;
}

SystemFault_t Control_GetSystemFault(void) {
    return g_systemFault;
}
