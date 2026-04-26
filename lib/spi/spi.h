#pragma once
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define SPI_DDR DDRB
#define MOSI PB3
#define MISO PB4
#define SCK PB5
#define CS PB2

void spi_init();
uint8_t spi_write(uint8_t data);
uint8_t spi_read();