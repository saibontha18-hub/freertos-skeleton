# freertos-skeleton

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE) [![Language](https://img.shields.io/badge/language-C-blue.svg)](main.c) [![FreeRTOS](https://img.shields.io/badge/FreeRTOS-Cortex--M-9cf.svg)](#)

A FreeRTOS task skeleton for Cortex-M. These are the patterns I end up rewriting in every firmware project — periodic tasks, queue-based messaging, a software timer, and a heartbeat watchdog — so I finally put them in one place. All standard FreeRTOS API, nothing port-specific.

## Tasks and timer

- `vLedTask` — blinks LED1 every 500 ms with `vTaskDelay`.
- `vCmdTask` — queue-based command handler: receives text commands via `xCmdQueue` and dispatches `LED ON` / `LED OFF` / `STATUS`. Uses a 1 s receive timeout instead of `portMAX_DELAY` so the heartbeat keeps ticking when no commands are coming in.
- `vSensorTask` — samples `board_adc_read()` every 250 ms into `xSampleQueue`. If the queue fills up it drops the sample rather than stalling the cadence.
- `vLogTask` — drains `xSampleQueue` and prints `adc=<raw> tick=<n>` lines over UART.
- `vWatchdogTask` — every 2 s, checks that every task's heartbeat counter moved since the last check; panics naming the hung task if one didn't. A 5 s grace period at startup lets slow starters check in first.
- Software timer `led2` — auto-reload timer toggling LED2 every 1250 ms, independent of the blinker task. The callback runs in the timer daemon context, so it must never block.

> **Does not compile standalone.** This skeleton intentionally excludes the FreeRTOS kernel sources, the port layer, and the MCU startup/linker files — those come from your toolchain and board. See "What you must provide" below.

## Intended target

- MCU: Cortex-M4F (example: STM32F407 on an STM32F4-Discovery board)
- FreeRTOS: FreeRTOS-Kernel v10.x (the API used here is stable across v10/v11)
- Toolchain: `arm-none-eabi-gcc`

## What you must provide

1. **FreeRTOS-Kernel sources** — `tasks.c`, `queue.c`, `list.c`, `timers.c` (event_groups.c optional), plus the port: `portable/GCC/ARM_CM4F/port.c`, `portable/GCC/ARM_CM4F/portmacro.h`, and a heap implementation such as `portable/MemMang/heap_4.c`.
2. **`FreeRTOSConfig.h`** — a template is included in this repo; copy it and adjust `configCPU_CLOCK_HZ`, heap size, and NVIC priorities for your chip.
3. **`board.c`** — implements the seven functions declared in `board.h` for your hardware:
   - `board_init`, `board_led_toggle`, `board_led_set`, `board_uart_puts` (clocks, GPIO, UART)
   - `board_adc_read` — sample the ADC channel feeding `vSensorTask`
   - `board_led2_toggle` — second LED, driven by the software timer
   - `board_panic` — fatal-error handler (log the reason, then halt or reset; must not return)
4. **Startup code and linker script** for your MCU (usually from STM32Cube or your vendor pack).
5. **UART RX path** — to feed live commands, call `xQueueSendFromISR(xCmdQueue, ...)` from your UART receive ISR. Until then, the demo commands seeded in `main()` exercise the handler.

## Build (example, after providing the above)

```sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
    -O2 -g -Wall \
    -I. -I/path/to/FreeRTOS-Kernel/include \
    -I/path/to/FreeRTOS-Kernel/portable/GCC/ARM_CM4F \
    main.c board.c startup_stm32f407xx.s \
    /path/to/FreeRTOS-Kernel/tasks.c \
    /path/to/FreeRTOS-Kernel/queue.c \
    /path/to/FreeRTOS-Kernel/list.c \
    /path/to/FreeRTOS-Kernel/timers.c \
    /path/to/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c \
    /path/to/FreeRTOS-Kernel/portable/MemMang/heap_4.c \
    -T STM32F407VGTx_FLASH.ld -nostartfiles \
    -o firmware.elf
```

Adjust CPU flags, include paths, and the linker script for your exact part.

## API used

`xTaskCreate`, `vTaskStartScheduler`, `vTaskDelay`, `pdMS_TO_TICKS`, `xQueueCreate`, `xQueueSend`, `xQueueReceive`, `portMAX_DELAY`, `configASSERT`, `xPortGetFreeHeapSize`, `xTaskGetTickCount`, `xTimerCreate`, `xTimerStart` — all standard, portable across FreeRTOS ports.

## Files

- `main.c` — tasks, software timer, and heartbeat watchdog
- `board.h` — BSP interface you implement per board (LEDs, UART, ADC, panic)
- `FreeRTOSConfig.h` — template configuration (adapt to your MCU; software timers enabled)

## License

MIT
