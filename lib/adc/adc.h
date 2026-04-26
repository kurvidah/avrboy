#pragma once
#include <avr/io.h>

void adc_init(uint8_t mask);
uint16_t adc_read(uint8_t channel);