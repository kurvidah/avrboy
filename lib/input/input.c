#include "input.h"
#include "event.h"
#include "adc.h"
#include <avr/io.h>
#include <stdlib.h>

static uint8_t last_btn_state = 0;

void input_init(void) {
    // 1. Setup PORTC Buttons (PC2, PC3)
    DDRC  &= ~(PORTC_MASK_UP | PORTC_MASK_DOWN);
    PORTC |=  (PORTC_MASK_UP | PORTC_MASK_DOWN); // Pull-ups

    // 2. Setup PORTD Buttons (PD2, PD5, PD6, PD7)
    DDRD  &= ~(PORTD_MASK_LEFT | PORTD_MASK_RIGHT | PORTD_MASK_A | PORTD_MASK_B);
    PORTD |=  (PORTD_MASK_LEFT | PORTD_MASK_RIGHT | PORTD_MASK_A | PORTD_MASK_B); // Pull-ups
    
    // 3. ADC initialized for channels 0 and 1 (Joystick X/Y)
    adc_init(0b00000011); 
}

void input_poll(void) {
    // --- 1. Gather Physical States ---
    uint8_t current_btns = 0;
    
    // Read PORTC bits
    uint8_t pinc = PINC;
    if (!(pinc & PORTC_MASK_UP))   current_btns |= BTN_UP;
    if (!(pinc & PORTC_MASK_DOWN)) current_btns |= BTN_DOWN;

    // Read PORTD bits
    uint8_t pind = PIND;
    if (!(pind & PORTD_MASK_LEFT))  current_btns |= BTN_LEFT;
    if (!(pind & PORTD_MASK_RIGHT)) current_btns |= BTN_RIGHT;
    if (!(pind & PORTD_MASK_A))     current_btns |= BTN_A;
    if (!(pind & PORTD_MASK_B))     current_btns |= BTN_B;

    // --- 2. Event Generation ---
    uint8_t changed_down = current_btns & ~last_btn_state;
    if (changed_down) {
        event_push(EVENT_BTN_DOWN, current_btns, 0);
    }

    uint8_t changed_up = ~current_btns & last_btn_state;
    if (changed_up) {
        event_push(EVENT_BTN_UP, current_btns, 0);
    }

    last_btn_state = current_btns;

    // --- 3. Analog Joystick (Throttled) ---
    static uint8_t adc_counter = 0;
    if (++adc_counter < 4) return;
    adc_counter = 0;

    static uint16_t last_x = 512, last_y = 512;
    uint16_t x = 1024 - adc_read(0); // PC0
    uint16_t y = 1024 - adc_read(1); // PC1

    if (abs((int16_t)x - (int16_t)last_x) > 32 || abs((int16_t)y - (int16_t)last_y) > 32) {
        event_push(EVENT_JOY_MOVE, (uint8_t)(x >> 2), (uint8_t)(y >> 2));
        last_x = x;
        last_y = y;
    }
}
