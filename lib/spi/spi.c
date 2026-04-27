#include "spi.h"
#include <avr/io.h>
#include <util/delay.h>

void spi_init(void) {
    // Set MOSI, SCK, CS as output
    SPI_DDR |= (1 << MOSI) | (1 << SCK) | (1 << CS);
    // Set MISO as input with pull-up
    SPI_DDR &= ~(1 << MISO);
    SPI_PORT |= (1 << MISO);

    // Enable SPI, Master, set clock rate fosc/64 (slow for init)
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1);
    
    // Ensure CS is high
    SPI_PORT |= (1 << CS);

    spi_set_speed(0); // Start slow
}

void spi_set_speed(uint8_t fast) {
    if (fast) {
        // Fast Mode: Aim for ~4MHz (Safe for LCD and SD)
        #if F_CPU >= 16000000UL
            // 16MHz / 4 = 4MHz
            SPCR &= ~((1 << SPR1) | (1 << SPR0));
            SPSR &= ~(1 << SPI2X);
        #else
            // 8MHz / 2 = 4MHz
            SPCR &= ~((1 << SPR1) | (1 << SPR0));
            SPSR |= (1 << SPI2X);
        #endif
    } else {
        // Slow Mode: Target ~250kHz for SD initialization
        #if F_CPU >= 16000000UL
            // 16MHz / 64 = 250kHz
            SPCR |= (1 << SPR1);
            SPCR &= ~(1 << SPR0);
            SPSR &= ~(1 << SPI2X);
        #else
            // 8MHz / 32 = 250kHz
            SPCR |= (1 << SPR1);
            SPCR &= ~(1 << SPR0);
            SPSR |= (1 << SPI2X);
        #endif
    }
    _delay_ms(1);
}

uint8_t spi_write(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

uint8_t spi_read(void) {
    return spi_write(0xFF);
}
