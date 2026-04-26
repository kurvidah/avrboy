#pragma once
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define MOSI PB3
#define MISO PB4
#define SCK PB5
#define CS PB2

void spi_init();
void spi_set_speed(uint8_t fast);
uint8_t spi_write(uint8_t data);
uint8_t spi_read();