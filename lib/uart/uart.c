#include "uart.h"
#include <avr/io.h>
#include <stdarg.h>
#include <stdio.h>

void uart_init(uint32_t baud) {
    uint16_t ubrr = (uint16_t)((F_CPU / (16UL * baud)) - 1);
    
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Enable transmitter and receiver
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    // Set frame format: 8 data, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putchar(char c) {
    if (c == '\n') {
        uart_putchar('\r');
    }
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));
    // Put data into buffer, sends the data
    UDR0 = c;
}

void uart_putstring(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

#ifdef DEBUG
void uart_log(const char *fmt, ...) {
    char buffer[48]; // Reduced buffer to conserve SRAM
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    uart_putstring(buffer);
    uart_putstring("\n");
}
#endif
