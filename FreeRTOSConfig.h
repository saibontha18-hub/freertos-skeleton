/*
 * FreeRTOSConfig.h - TEMPLATE for a Cortex-M4 target (e.g. STM32F4).
 *
 * This is a starting point, not a finished configuration. Copy it into your
 * project and adjust at least:
 *   - configCPU_CLOCK_HZ to your core clock
 *   - configTICK_RATE_HZ if you need a different tick rate
 *   - configTOTAL_HEAP_SIZE to fit your RAM budget
 *   - configLIBRARY_LOWEST_INTERRUPT_PRIORITY / MAX_SYSCALL priority to
 *     match your NVIC priority grouping (see the FreeRTOS Cortex-M docs)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( 168000000UL )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 20 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configGENERATE_RUN_TIME_STATS           0

/* Cortex-M4F specific: enable the FPU lazy stacking in the port. */
#define configENABLE_FPU                        1

/* API inclusion */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* Software timers: used for the LED2 blink timer in main.c. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* NVIC priority configuration for STM32 (4 priority bits). */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Map the port's interrupt handlers to the CMSIS names used in the
 * STM32 startup file. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
