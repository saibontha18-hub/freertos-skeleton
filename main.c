/*
 * main.c - Minimal FreeRTOS task skeleton for Cortex-M.
 *
 * Three tasks, all standard FreeRTOS API:
 *   vLedTask - blinks the board LED every 500 ms (vTaskDelay).
 *   vCmdTask - queue-based command handler: receives text commands from
 *              xCmdQueue and acts on them ("LED ON", "LED OFF", "STATUS").
 *   (UART RX ISR not included: wire your UART receive interrupt to call
 *    xQueueSendFromISR(xCmdQueue, ...) to feed vCmdTask on real hardware.)
 *
 * A couple of demo commands are seeded at startup so the handler path can
 * be observed even before the UART RX path is wired up.
 */

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"

#define LED_TASK_PRIO    ( tskIDLE_PRIORITY + 1 )
#define CMD_TASK_PRIO    ( tskIDLE_PRIORITY + 2 )

#define LED_TASK_STACK   ( configMINIMAL_STACK_SIZE )
#define CMD_TASK_STACK   ( configMINIMAL_STACK_SIZE * 2 )

#define CMD_QUEUE_LEN    8
#define CMD_MAX_LEN      32

typedef struct
{
    char text[CMD_MAX_LEN];
} cmd_t;

static QueueHandle_t xCmdQueue = NULL;

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

int main(void)
{
    cmd_t demo;

    board_init();

    xCmdQueue = xQueueCreate(CMD_QUEUE_LEN, sizeof(cmd_t));
    configASSERT(xCmdQueue != NULL);

    xTaskCreate(vLedTask, "led", LED_TASK_STACK, NULL,
                LED_TASK_PRIO, NULL);
    xTaskCreate(vCmdTask, "cmd", CMD_TASK_STACK, NULL,
                CMD_TASK_PRIO, NULL);

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
