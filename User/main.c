#include "stm32f10x.h"                  // Device header
#include "Control.h"
#include "TempHeat.h"
#include "Pressure.h"
#include "Level.h"
#include "Display.h"
#include "EL.h"
#include "Serial.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"

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

static StaticTask_t g_idleTaskTcb;
static StackType_t g_idleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **tcbBuffer,
	StackType_t **stackBuffer,
	uint32_t *stackSize) {
	*tcbBuffer = &g_idleTaskTcb;
	*stackBuffer = g_idleTaskStack;
	*stackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *taskName) {
	(void)task;
	(void)taskName;
	TempHeat_SetPower(0);
	EL_Disable();
	portDISABLE_INTERRUPTS();
	for (;;) {
	}
}

int main(void)
{
	/* Initialize hardware before creating the first task. */
	Timer2_Init();
	TempHeat_Init();
	Pressure_Init();
	Level_Init();
	EL_Init();
	Control_Init();
	Display_Init();
	Serial_Init();
	AppTasks_Create();

	vTaskStartScheduler();

	TempHeat_SetPower(0);
	EL_Disable();
	for (;;) {
	}
}
