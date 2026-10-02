#ifndef BOARD_H
#define BOARD_H

/*
 * board.h - Board Support Package interface.
 *
 * YOU must implement these functions for your MCU/board in a board.c file.
 * The examples below assume an STM32F4-style target, but any Cortex-M
 * with a GPIO LED and a UART will do.
 */

#include <stdint.h>

void board_init(void);              /* clocks, GPIO, UART init */
void board_led_toggle(void);        /* toggle the status LED */
void board_led_set(int on);         /* on != 0 -> LED on, else off */
void board_uart_puts(const char *s);/* blocking UART transmit of a C string */
uint16_t board_adc_read(void);      /* sample the ADC channel (e.g. sensor) */
void board_led2_toggle(void);       /* toggle the second LED (timer-driven) */

#endif /* BOARD_H */
