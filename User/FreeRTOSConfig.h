#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f10x.h"

#define __dsb( ... )                            __DSB()
#define __isb( ... )                            __ISB()
#define __clz( value )                          __builtin_clz( value )

#define configCPU_CLOCK_HZ                         ( 72000000UL )
#define configSYSTICK_CLOCK_HZ                    ( configCPU_CLOCK_HZ )
#define configTICK_RATE_HZ                        ( 1000U )
#define configTICK_TYPE_WIDTH_IN_BITS             TICK_TYPE_WIDTH_32_BITS
#define configMAX_PRIORITIES                      5
#define configMINIMAL_STACK_SIZE                  128
#define configMAX_TASK_NAME_LEN                   16
#define configUSE_PREEMPTION                      1
#define configUSE_TIME_SLICING                    1
#define configUSE_IDLE_HOOK                       0
#define configUSE_TICK_HOOK                       0
#define configSUPPORT_STATIC_ALLOCATION           1
#define configSUPPORT_DYNAMIC_ALLOCATION          0
#define configKERNEL_PROVIDED_STATIC_MEMORY       0
#define configUSE_MUTEXES                         0
#define configUSE_RECURSIVE_MUTEXES               0
#define configUSE_COUNTING_SEMAPHORES             0
#define configUSE_QUEUE_SETS                      0
#define configUSE_TIMERS                          0
#define configUSE_CO_ROUTINES                     0
#define configCHECK_FOR_STACK_OVERFLOW            2

#define configPRIO_BITS                           4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY           ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configCHECK_HANDLER_INSTALLATION          1

#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskDelayUntil                  1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_uxTaskGetStackHighWaterMark      1

#define configASSERT( expression )                \
    if ( ( expression ) == 0 )                    \
    {                                             \
        taskDISABLE_INTERRUPTS();                 \
        for ( ;; )                                 \
        {                                         \
        }                                         \
    }

/* FreeRTOS exception handlers must keep the port's naked assembly bodies. */
#define vPortSVCHandler                          SVC_Handler
#define xPortPendSVHandler                       PendSV_Handler
#define xPortSysTickHandler                      SysTick_Handler

#endif