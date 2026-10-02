#ifndef BOARD_H
#define BOARD_H

/*
 * board.h - the BSP interface. Implement these in board.c for your MCU.
 * Written against an STM32F4-style target, but any Cortex-M with a GPIO
 * LED and a UART will do.
 */

#include <stdint.h>

void board_init(void);              /* clocks, GPIO, UART init */
void board_led_toggle(void);        /* toggle the status LED */
void board_led_set(int on);         /* on != 0 -> LED on, else off */
void board_uart_puts(const char *s);/* blocking UART transmit of a C string */
uint16_t board_adc_read(void);      /* sample the ADC channel (e.g. sensor) */
void board_led2_toggle(void);       /* toggle the second LED (timer-driven) */
void board_panic(const char *reason); /* fatal: log the reason, then halt
                                       * or reset. Must not return. */

#endif /* BOARD_H */
