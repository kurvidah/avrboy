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

    // Enable SPI, Master mode
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA) ;

    // Keep SPI clock at ~2MHz
    if (F_CPU > 8000000UL) { // fosc/4
        SPCR |= (1 << SPR0); 
    } else {
        SPSR |= (1 << SPI2X); // fosc/2
    }
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