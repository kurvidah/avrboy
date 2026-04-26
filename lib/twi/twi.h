#pragma once

#include <stdint.h>

#define TWI_READ  1
#define TWI_WRITE 0

void twi_init(uint32_t freq);
uint8_t twi_start(uint8_t address);
uint8_t twi_write(uint8_t data);
uint8_t twi_read_ack(void);
uint8_t twi_read_nack(void);
void twi_stop(void);

