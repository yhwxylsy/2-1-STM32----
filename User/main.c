#include "stm32f10x.h"                  // Device header
#include "Control.h"
#include "TempHeat.h"
#include "Pressure.h"
#include "Level.h"
#include "Display.h"
#include "EL.h"
#include "Serial.h"

volatile uint32_t timer_seconds = 0; /* Updated by TIM2_IRQHandler; unit is seconds. */

/**
	* @brief  TIM2 timebase initialization
  * @param  无
  * @retval 无
  */
void Timer2_Init(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_Period = 7199;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 9999;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    
    // 使能定时器更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

/* 共用串口状态格式，避免周期上报与's'命令内容分叉。 */
static void Serial_SendSystemStatus(void) {
	uint8_t pressureDetected;

	Serial_SendString((uint8_t*)"System Status:\r\n");
	Serial_SendString((uint8_t*)"Current Temp: ");
	Serial_SendNumber(Control_GetCurrentTemp(), 2);
	Serial_SendString((uint8_t*)"C\r\nState: ");
	switch (Control_GetSystemState()) {
		case SYSTEM_OFF:
			Serial_SendString((uint8_t*)"OFF\r\n");
			break;
		case SYSTEM_STANDBY:
			Serial_SendString((uint8_t*)"STANDBY\r\n");
			break;
		case SYSTEM_HEATING:
			Serial_SendString((uint8_t*)"HEATING\r\n");
			break;
		case SYSTEM_KEEPING:
			Serial_SendString((uint8_t*)"KEEPING\r\n");
			break;
		case SYSTEM_ERROR:
			Serial_SendString((uint8_t*)"ERROR\r\n");
			break;
		default:
			Serial_SendString((uint8_t*)"UNKNOWN\r\n");
			break;
	}

	Serial_SendString((uint8_t*)"Fault: ");
	switch (Control_GetSystemFault()) {
		case SYSTEM_FAULT_TEMPERATURE:
			Serial_SendString((uint8_t*)"Temperature\r\n");
			break;
		case SYSTEM_FAULT_PRESSURE:
			Serial_SendString((uint8_t*)"Pressure Sensor\r\n");
			break;
		case SYSTEM_FAULT_LIQUID_LEVEL:
			Serial_SendString((uint8_t*)"Liquid Level\r\n");
			break;
		case SYSTEM_FAULT_NONE:
		default:
			Serial_SendString((uint8_t*)"None\r\n");
			break;
	}

	Serial_SendString((uint8_t*)"Liquid Level: ");
	Serial_SendString(Level_Detect() ? (uint8_t*)"Present\r\n" : (uint8_t*)"Absent\r\n");

	Serial_SendString((uint8_t*)"Pressure: ");
	if (!Pressure_Detect(&pressureDetected)) {
		Serial_SendString((uint8_t*)"Sensor Error\r\n");
	} else if (pressureDetected) {
		Serial_SendString((uint8_t*)"Detected\r\n");
	} else {
		Serial_SendString((uint8_t*)"Not Detected\r\n");
	}

	Serial_SendString((uint8_t*)"--------------------\r\n");
}

int main(void)
{
	uint32_t lastControlTime = 0;
	uint32_t lastButtonScanTime = 0;
	uint32_t lastStatusTime = 0;

	// 初始化TIM2时基
	Timer2_Init();
	
	// 初始化各个模块
	TempHeat_Init();
	Pressure_Init();
	Level_Init();
	EL_Init();
	Control_Init();
	Display_Init();
	Serial_Init();  // 初始化串口通信
	
	// 主循环
	while (1)
	{
		uint32_t currentTime = GetTick();

		/* 固定控制周期，避免温控频率随主循环空转速度变化。 */
		if (currentTime - lastControlTime >= 100U) {
			lastControlTime = currentTime;
			Control_Update();
		}

		/* 按键扫描周期独立于较慢的温控周期。 */
		if (currentTime - lastButtonScanTime >= 10U) {
			lastButtonScanTime = currentTime;
			Display_CheckButtons();
		}

		// 每秒发送一次状态
		if (currentTime - lastStatusTime >= 1000U) {
			lastStatusTime = currentTime;
			Serial_SendSystemStatus();
		}
		
		// 处理串口接收的数据
		if (Serial_GetRxFlag()) {
			uint8_t rxData = Serial_ReceiveByte();
			
			// 根据接收到的命令执行相应操作
			switch (rxData) {
				case 's':  // 发送系统状态
					Serial_SendSystemStatus();
					break;
				case 'h':  // 发送帮助信息
					Serial_SendString((uint8_t*)"Help:\r\n");
					Serial_SendString((uint8_t*)"s - Send system status\r\n");
					Serial_SendString((uint8_t*)"h - Show help\r\n");
					break;
				default:
					Serial_SendString((uint8_t*)"Unknown command. Type 'h' for help.\r\n");
					break;
			}
		}
	}
}
