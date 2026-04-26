#pragma once

#include <stdint.h>
#include <stdio.h>

// Initialize UART with a specific baud rate
void uart_init(uint32_t baud);

// Send a single character
void uart_putchar(char c);

// Send a string
void uart_putstring(const char *s);

// A simple printf wrapper (uses a fixed buffer to save memory)
#ifdef DEBUG
void uart_log(const char *fmt, ...);
#else
#define uart_log(fmt, ...) ((void)0)
#endif

