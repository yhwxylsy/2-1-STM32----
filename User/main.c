#include "stm32f10x.h"                  // Device header
#include "Control.h"
#include "TempHeat.h"
#include "Pressure.h"
#include "Level.h"
#include "Display.h"
#include "EL.h"
#include "Serial.h"

// 定时器中断标志
uint8_t timer_flag = 0;

/**
  * @brief  系统时钟初始化
  * @param  无
  * @retval 无
  */
void SystemClock_Init(void) {
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

int main(void)
{
	// 初始化系统时钟
	SystemClock_Init();
	
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
		// 更新系统状态
		Control_Update();
		
		// 检查按键
		Display_CheckButtons();
		
		// 当定时器中断标志设置时，发送数据
		if (timer_flag) {
			// 通过串口发送传感器数据
			Serial_SendString((uint8_t*)"System Status:\r\n");
			Serial_SendString((uint8_t*)"Current Temp: ");
			Serial_SendNumber(Control_GetCurrentTemp(), 2);
			Serial_SendString((uint8_t*)"C\r\n");
			
			Serial_SendString((uint8_t*)"Liquid Level: ");
			if (Level_Detect()) {
				Serial_SendString((uint8_t*)"Present\r\n");
			} else {
				Serial_SendString((uint8_t*)"Absent\r\n");
			}
			
			Serial_SendString((uint8_t*)"Pressure: ");
			if (Pressure_Detect()) {
				Serial_SendString((uint8_t*)"Detected\r\n");
			} else {
				Serial_SendString((uint8_t*)"Not Detected\r\n");
			}
			
			Serial_SendString((uint8_t*)"--------------------\r\n");
			
			// 清除中断标志
			timer_flag = 0;
		}
		
		// 处理串口接收的数据
		if (Serial_GetRxFlag()) {
			uint8_t rxData = Serial_ReceiveByte();
			
			// 根据接收到的命令执行相应操作
			switch (rxData) {
				case 's':  // 发送系统状态
					Serial_SendString((uint8_t*)"System Status:\r\n");
					Serial_SendString((uint8_t*)"Current Temp: ");
					Serial_SendNumber(Control_GetCurrentTemp(), 2);
					Serial_SendString((uint8_t*)"C\r\n");
					Serial_SendString((uint8_t*)"Liquid Level: ");
					if (Level_Detect()) {
						Serial_SendString((uint8_t*)"Present\r\n");
					} else {
						Serial_SendString((uint8_t*)"Absent\r\n");
					}
					Serial_SendString((uint8_t*)"Pressure: ");
					if (Pressure_Detect()) {
						Serial_SendString((uint8_t*)"Detected\r\n");
					} else {
						Serial_SendString((uint8_t*)"Not Detected\r\n");
					}
					Serial_SendString((uint8_t*)"--------------------\r\n");
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
