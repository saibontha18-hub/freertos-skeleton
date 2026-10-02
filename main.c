/*
 * main.c - FreeRTOS task skeleton for Cortex-M.
 *
 * Tasks, all standard FreeRTOS API:
 *   vLedTask    - blinks the board LED every 500 ms (vTaskDelay).
 *   vCmdTask    - queue-based command handler: receives text commands from
 *                 xCmdQueue and acts on them ("LED ON", "LED OFF", "STATUS").
 *   vSensorTask - samples board_adc_read() every 250 ms and pushes the
 *                 readings into xSampleQueue.
 *   vLogTask    - consumes xSampleQueue and prints "adc=<raw> tick=<n>"
 *                 lines over UART.
 *
 * Plus a software timer (vLed2TimerCallback) toggling LED2 every 1250 ms,
 * independent of the vLedTask blink rate.
 * (UART RX ISR not included: wire your UART receive interrupt to call
 *  xQueueSendFromISR(xCmdQueue, ...) to feed vCmdTask on real hardware.)
 *
 * A couple of demo commands are seeded at startup so the handler path can
 * be observed even before the UART RX path is wired up.
 */

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "board.h"

#define LED_TASK_PRIO    ( tskIDLE_PRIORITY + 1 )
#define CMD_TASK_PRIO    ( tskIDLE_PRIORITY + 2 )
#define SENSOR_TASK_PRIO ( tskIDLE_PRIORITY + 2 )
#define LOG_TASK_PRIO    ( tskIDLE_PRIORITY + 1 )

#define LED_TASK_STACK    ( configMINIMAL_STACK_SIZE )
#define CMD_TASK_STACK    ( configMINIMAL_STACK_SIZE * 2 )
#define SENSOR_TASK_STACK ( configMINIMAL_STACK_SIZE )
#define LOG_TASK_STACK    ( configMINIMAL_STACK_SIZE * 2 )

#define CMD_QUEUE_LEN    8
#define CMD_MAX_LEN      32
#define SAMPLE_QUEUE_LEN 16

#define LED2_TIMER_PERIOD_MS 1250

typedef struct
{
    char text[CMD_MAX_LEN];
} cmd_t;

typedef struct
{
    TickType_t xTick;   /* xTaskGetTickCount() at sampling time */
    uint16_t usAdc;     /* raw ADC sample from board_adc_read() */
} sample_t;

static QueueHandle_t xCmdQueue = NULL;
static QueueHandle_t xSampleQueue = NULL;
static TimerHandle_t xLed2Timer = NULL;

/* Software-timer callback: runs in the timer daemon task context, so it
 * must never block. Toggling a GPIO is fine. */
static void vLed2TimerCallback(TimerHandle_t xTimer)
{
    (void) xTimer;
    board_led2_toggle();
}

static void vLedTask(void *pvParameters)
{
    (void) pvParameters;

    for (;;)
    {
        board_led_toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vCmdTask(void *pvParameters)
{
    cmd_t cmd;
    char reply[64];

    (void) pvParameters;

    for (;;)
    {
        if (xQueueReceive(xCmdQueue, &cmd, portMAX_DELAY) == pdPASS)
        {
            cmd.text[CMD_MAX_LEN - 1] = '\0';

            if (strncmp(cmd.text, "LED ON", 6) == 0)
            {
                board_led_set(1);
                board_uart_puts("OK: LED on\r\n");
            }
            else if (strncmp(cmd.text, "LED OFF", 7) == 0)
            {
                board_led_set(0);
                board_uart_puts("OK: LED off\r\n");
            }
            else if (strncmp(cmd.text, "STATUS", 6) == 0)
            {
                snprintf(reply, sizeof(reply),
                         "OK: tasks running, free heap=%u\r\n",
                         (unsigned) xPortGetFreeHeapSize());
                board_uart_puts(reply);
            }
            else
            {
                board_uart_puts("ERR: unknown command\r\n");
            }
        }
    }
}

static void vSensorTask(void *pvParameters)
{
    sample_t xSample;

    (void) pvParameters;

    for (;;)
    {
        xSample.xTick = xTaskGetTickCount();
        xSample.usAdc = board_adc_read();

        /* 0-tick wait: samples are periodic; dropping one under backpressure
         * is better than stalling the sampling cadence. */
        (void) xQueueSend(xSampleQueue, &xSample, 0);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void vLogTask(void *pvParameters)
{
    sample_t xSample;
    char pcLine[64];

    (void) pvParameters;

    for (;;)
    {
        /* 1 s timeout instead of portMAX_DELAY: stays responsive and gives
         * the task a chance to do periodic work even when no samples arrive. */
        if (xQueueReceive(xSampleQueue, &xSample, pdMS_TO_TICKS(1000)) == pdPASS)
        {
            snprintf(pcLine, sizeof(pcLine), "adc=%u tick=%lu\r\n",
                     (unsigned) xSample.usAdc,
                     (unsigned long) xSample.xTick);
            board_uart_puts(pcLine);
        }
    }
}

int main(void)
{
    cmd_t demo;

    board_init();

    xCmdQueue = xQueueCreate(CMD_QUEUE_LEN, sizeof(cmd_t));
    configASSERT(xCmdQueue != NULL);

    xSampleQueue = xQueueCreate(SAMPLE_QUEUE_LEN, sizeof(sample_t));
    configASSERT(xSampleQueue != NULL);

    xTaskCreate(vLedTask, "led", LED_TASK_STACK, NULL,
                LED_TASK_PRIO, NULL);
    xTaskCreate(vCmdTask, "cmd", CMD_TASK_STACK, NULL,
                CMD_TASK_PRIO, NULL);
    xTaskCreate(vSensorTask, "sensor", SENSOR_TASK_STACK, NULL,
                SENSOR_TASK_PRIO, NULL);
    xTaskCreate(vLogTask, "log", LOG_TASK_STACK, NULL,
                LOG_TASK_PRIO, NULL);

    /* Auto-reload software timer: LED2 blinks at its own rate,
     * independent of the vLedTask cadence. */
    xLed2Timer = xTimerCreate("led2",
                              pdMS_TO_TICKS(LED2_TIMER_PERIOD_MS),
                              pdTRUE,
                              NULL,
                              vLed2TimerCallback);
    configASSERT(xLed2Timer != NULL);
    configASSERT(xTimerStart(xLed2Timer, 0) == pdPASS);

    /* Seed demo commands so the handler path is exercised even with no
     * UART RX wired up yet. Remove once xQueueSendFromISR feeds the queue. */
    strncpy(demo.text, "STATUS", sizeof(demo.text));
    xQueueSend(xCmdQueue, &demo, 0);

    vTaskStartScheduler();

    /* Should never reach here. */
    for (;;)
    {
    }
}
