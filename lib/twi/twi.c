#include "twi.h"
#include <avr/io.h>

#define TWI_TIMEOUT 10000

void twi_init(uint32_t freq) {
    // SCL freq = F_CPU / (16 + 2 * TWBR * Prescaler)
    // TWBR = ((F_CPU / freq) - 16) / 2
    uint32_t val = (F_CPU / freq);
    if (val > 16) {
        TWBR = (uint8_t)((val - 16) / 2);
    } else {
        TWBR = 0;
    }
    TWSR = 0; // Prescaler = 1
}

uint8_t twi_start(uint8_t address) {
    uint16_t timeout = TWI_TIMEOUT;
    // Send START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)) && --timeout);
    if (timeout == 0) return 0xFF;

    // Send address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);
    timeout = TWI_TIMEOUT;
    while (!(TWCR & (1 << TWINT)) && --timeout);
    if (timeout == 0) return 0xFF;

    // Return status
    return (TWSR & 0xF8);
}

uint8_t twi_write(uint8_t data) {
    uint16_t timeout = TWI_TIMEOUT;
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)) && --timeout);
    if (timeout == 0) return 0xFF;
    return (TWSR & 0xF8);
}

uint8_t twi_read_ack(void) {
    uint16_t timeout = TWI_TIMEOUT;
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)) && --timeout);
    return TWDR;
}

uint8_t twi_read_nack(void) {
    uint16_t timeout = TWI_TIMEOUT;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)) && --timeout);
    return TWDR;
}

void twi_stop(void) {
    uint16_t timeout = TWI_TIMEOUT;
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    while ((TWCR & (1 << TWSTO)) && --timeout);
}
