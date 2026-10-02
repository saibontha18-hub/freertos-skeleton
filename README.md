# freertos-skeleton

A minimal FreeRTOS task skeleton for Cortex-M microcontrollers, using only standard FreeRTOS API calls. It demonstrates three common patterns: a periodic task (`vLedTask` blinks an LED with `vTaskDelay`), a queue-based command handler (`vCmdTask` receives text commands via `xQueueReceive` and dispatches `LED ON` / `LED OFF` / `STATUS`), and debug output over UART through a small board-support interface (`board.h`).

> **Does not compile standalone.** This skeleton intentionally excludes the FreeRTOS kernel sources, the port layer, and the MCU startup/linker files — those come from your toolchain and board. See "What you must provide" below.

## Intended target

- MCU: Cortex-M4F (example: STM32F407 on an STM32F4-Discovery board)
- FreeRTOS: FreeRTOS-Kernel v10.x (the API used here is stable across v10/v11)
- Toolchain: `arm-none-eabi-gcc`

## What you must provide

1. **FreeRTOS-Kernel sources** — `tasks.c`, `queue.c`, `list.c`, `timers.c` (event_groups.c optional), plus the port: `portable/GCC/ARM_CM4F/port.c`, `portable/GCC/ARM_CM4F/portmacro.h`, and a heap implementation such as `portable/MemMang/heap_4.c`.
2. **`FreeRTOSConfig.h`** — a template is included in this repo; copy it and adjust `configCPU_CLOCK_HZ`, heap size, and NVIC priorities for your chip.
3. **`board.c`** — implements the four functions declared in `board.h` (`board_init`, `board_led_toggle`, `board_led_set`, `board_uart_puts`) for your hardware.
4. **Startup code and linker script** for your MCU (usually from STM32Cube or your vendor pack).
5. **UART RX path** — to feed live commands, call `xQueueSendFromISR(xCmdQueue, ...)` from your UART receive ISR. Until then, the two demo commands seeded in `main()` exercise the handler.

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

`xTaskCreate`, `vTaskStartScheduler`, `vTaskDelay`, `pdMS_TO_TICKS`, `xQueueCreate`, `xQueueSend`, `xQueueReceive`, `portMAX_DELAY`, `configASSERT`, `xPortGetFreeHeapSize` — all standard, portable across FreeRTOS ports.

## Files

- `main.c` — tasks and command handler
- `board.h` — BSP interface you implement per board
- `FreeRTOSConfig.h` — template configuration (adapt to your MCU)

## License

MIT
