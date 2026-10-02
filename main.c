/*
 * FreeRTOS task skeleton for Cortex-M.
 *
 * The usual suspects: a blinker, a queue-driven command handler, a sensor
 * sampler feeding a logger task, a software timer, and a heartbeat
 * watchdog that panics if any task stops checking in.
 *
 * To feed commands from real hardware, hook your UART RX ISR up to
 * xQueueSendFromISR(xCmdQueue, ...). Until then, main() seeds a couple
 * of demo commands so you can watch the handler work.
 */

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "board.h"

#define LED_TASK_PRIO      ( tskIDLE_PRIORITY + 1 )
#define CMD_TASK_PRIO      ( tskIDLE_PRIORITY + 2 )
#define SENSOR_TASK_PRIO   ( tskIDLE_PRIORITY + 2 )
#define LOG_TASK_PRIO      ( tskIDLE_PRIORITY + 1 )
#define WATCHDOG_TASK_PRIO ( tskIDLE_PRIORITY + 3 )

#define LED_TASK_STACK      ( configMINIMAL_STACK_SIZE )
#define CMD_TASK_STACK      ( configMINIMAL_STACK_SIZE * 2 )
#define SENSOR_TASK_STACK   ( configMINIMAL_STACK_SIZE )
#define LOG_TASK_STACK      ( configMINIMAL_STACK_SIZE * 2 )
#define WATCHDOG_TASK_STACK ( configMINIMAL_STACK_SIZE )

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

/* Heartbeat watchdog: each task bumps its counter once per loop.
 * The watchdog snapshots them every 2 s and panics on any counter that
 * stopped moving. 32-bit writes are atomic on Cortex-M, so no lock needed. */
typedef enum
{
    HB_LED = 0,
    HB_CMD,
    HB_SENSOR,
    HB_LOG,
    HB_COUNT
} hb_id_t;

static volatile uint32_t ulHeartbeats[HB_COUNT];

#define HEARTBEAT( id )                    \
    do { ulHeartbeats[ ( id ) ]++; } while ( 0 )

#define WATCHDOG_PERIOD_MS 2000
#define WATCHDOG_GRACE_MS  5000

/* Timer callbacks run in the daemon task: never block in here. */
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
        HEARTBEAT(HB_LED);
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
        /* 1 s timeout instead of portMAX_DELAY, so the heartbeat keeps
         * ticking even when no commands are coming in. */
        if (xQueueReceive(xCmdQueue, &cmd, pdMS_TO_TICKS(1000)) == pdPASS)
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

        HEARTBEAT(HB_CMD);
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

        /* Don't wait on a full queue: dropping a sample beats stalling
         * the sampling cadence. */
        (void) xQueueSend(xSampleQueue, &xSample, 0);

        HEARTBEAT(HB_SENSOR);
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
        /* 1 s timeout so the task stays responsive when samples dry up. */
        if (xQueueReceive(xSampleQueue, &xSample, pdMS_TO_TICKS(1000)) == pdPASS)
        {
            snprintf(pcLine, sizeof(pcLine), "adc=%u tick=%lu\r\n",
                     (unsigned) xSample.usAdc,
                     (unsigned long) xSample.xTick);
            board_uart_puts(pcLine);
        }

        HEARTBEAT(HB_LOG);
    }
}

static void vWatchdogTask(void *pvParameters)
{
    static const char * const pcTaskNames[HB_COUNT] =
    {
        "led", "cmd", "sensor", "log"
    };
    uint32_t ulLast[HB_COUNT];
    UBaseType_t uxId;

    (void) pvParameters;

    /* Let every task check in once before the first comparison. */
    vTaskDelay(pdMS_TO_TICKS(WATCHDOG_GRACE_MS));
    for (uxId = 0; uxId < HB_COUNT; uxId++)
    {
        ulLast[uxId] = ulHeartbeats[uxId];
    }

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_PERIOD_MS));

        for (uxId = 0; uxId < HB_COUNT; uxId++)
        {
            if (ulHeartbeats[uxId] == ulLast[uxId])
            {
                /* Hung task: no recovery, panic. */
                board_panic(pcTaskNames[uxId]);
            }
            ulLast[uxId] = ulHeartbeats[uxId];
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
    xTaskCreate(vWatchdogTask, "watchdog", WATCHDOG_TASK_STACK, NULL,
                WATCHDOG_TASK_PRIO, NULL);

    /* LED2 on its own timer, independent of the blinker task. */
    xLed2Timer = xTimerCreate("led2",
                              pdMS_TO_TICKS(LED2_TIMER_PERIOD_MS),
                              pdTRUE,
                              NULL,
                              vLed2TimerCallback);
    configASSERT(xLed2Timer != NULL);
    configASSERT(xTimerStart(xLed2Timer, 0) == pdPASS);

    /* Demo commands so the handler path runs before the UART RX ISR is
     * wired up. Delete once xQueueSendFromISR feeds the queue. */
    strncpy(demo.text, "STATUS", sizeof(demo.text));
    xQueueSend(xCmdQueue, &demo, 0);

    vTaskStartScheduler();

    /* Should never reach here. */
    for (;;)
    {
    }
}
