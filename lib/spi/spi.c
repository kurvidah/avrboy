#include "spi.h"
#include <avr/io.h>

#ifdef SPI_DEBUG
#include "uart.h"
static void uart_puthex(uint8_t n) {
    static const char hex[] = "0123456789ABCDEF";
    uart_putchar(hex[n >> 4]);
    uart_putchar(hex[n & 0x0F]);
}
#endif

void spi_init() {
    // Set MOSI, SCK as output; MISO as input
    SPI_DDR |= (1 << MOSI) | (1 << SCK) | (1 << CS);
    SPI_DDR &= ~(1 << MISO);
    SPI_PORT |= (1 << MISO); // Enable internal pull-up on MISO

    // Enable SPI, Master mode, Mode 0 (CPOL=0, CPHA=0)
    SPCR = (1 << SPE) | (1 << MSTR);
    
    spi_set_speed(0); // Start slow
}

void spi_set_speed(uint8_t fast) {
    if (fast) {
        // Fast: fosc/16 (1MHz @ 16MHz)
        SPCR |= (1 << SPR0);
        SPCR &= ~(1 << SPR1);
        SPSR &= ~(1 << SPI2X);
    } else {
        // Slow: fosc/64 (250kHz @ 16MHz)
        SPCR |= (1 << SPR1);
        SPCR &= ~(1 << SPR0);
        SPSR &= ~(1 << SPI2X);
    }
    _delay_ms(10);
}

uint8_t spi_write(uint8_t data) {
#ifdef SPI_DEBUG
    uart_putstring("SPI:");
    uart_puthex(data);
    uart_putchar('\n');
#endif
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

uint8_t spi_read() {
    return spi_write(0xFF); // Send dummy byte to clock in data
}