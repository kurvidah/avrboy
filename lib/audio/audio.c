#include "audio.h"
#include <avr/io.h>

void audio_init(void) {
    // Set PD3 (OC2B) as output
    DDRD |= (1 << PD3);
    
    // Timer 2: CTC Mode (Mode 2)
    // We'll use Toggle OC2B on Compare Match
    TCCR2A = (1 << COM2B0) | (1 << WGM21);
    TCCR2B = 0; // Stopped initially
}

void audio_play_tone(uint16_t freq) {
    if (freq == 0) {
        TCCR2B = 0; // Stop timer
        PORTD &= ~(1 << PD3);
        return;
    }

    // F_OC2B = F_CPU / (2 * N * (1 + OCR2A))
    // We'll use OCR2A to define the TOP value (frequency)
    
    uint32_t base = F_CPU / 2;
    uint32_t top = (base / 64 / freq);
    
    if (top > 256) {
        // Frequency too low for prescaler 64, try 256
        top = (base / 256 / freq);
        if (top > 256) {
            // Still too low, try 1024
            top = (base / 1024 / freq);
            OCR2A = (uint8_t)(top > 256 ? 255 : top - 1);
            TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // 1024
        } else {
            OCR2A = (uint8_t)(top - 1);
            TCCR2B = (1 << CS22) | (1 << CS21); // 256
        }
    } else {
        OCR2A = (uint8_t)(top - 1);
        TCCR2B = (1 << CS22); // 64
    }
}
