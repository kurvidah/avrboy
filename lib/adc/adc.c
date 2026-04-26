#include "adc.h"

void adc_init(uint8_t mask) {
  DDRC = DDRC & ~mask; // Set ADC pins as input

  ADCSRA |= (1 << ADEN);                                // Enable ADC
  ADCSRA &= ~(1 << ADIE);                               // Ensure ADC interrupt is disabled for polling
  ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2); // Prescaler
  ADMUX = (ADMUX & 0b111111) | 0b01 << 6;                 // Vref
  ADMUX &= ~(1 << 5);                                   // Right Adjust
}

uint16_t adc_read(uint8_t channel) {
  ADMUX = (ADMUX & 0b11110000) | (channel & 0b1111);

  // Pulse ADSC
  ADCSRA |= 1 << 6;
  while (ADCSRA & (1 << 6))
    ;

  // Read ADC
  return ADC;
}