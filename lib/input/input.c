#include "input.h"
#include "event.h"
#include "adc.h"
#include <avr/io.h>
#include <stdlib.h>

static uint8_t last_btn_state = 0;

void input_init(void) {
    // Set all 6 buttons (PC0-PC5) as input with pull-ups
    INPUT_DDR &= ~(MASK_UP | MASK_DOWN | MASK_LEFT | MASK_RIGHT | MASK_A | MASK_B);
    INPUT_PORT |= (MASK_UP | MASK_DOWN | MASK_LEFT | MASK_RIGHT | MASK_A | MASK_B);
    
    // ADC initialized for channels 6 and 7 (A6 and A7)
    adc_init(0b11000000); 
}

void input_poll(void) {
    // 1. Digital Buttons (Active Low)
    uint8_t current_raw = ~INPUT_PIN; // Invert so 1 = pressed
    uint8_t current_btns = current_raw & 0x3F; // Only mask PC0-PC5

    // Detect changes (Pressed)
    uint8_t changed_down = current_btns & ~last_btn_state;
    if (changed_down) {
        event_push(EVENT_BTN_DOWN, changed_down, 0);
    }

    // Detect changes (Released)
    uint8_t changed_up = ~current_btns & last_btn_state;
    if (changed_up) {
        event_push(EVENT_BTN_UP, changed_up, 0);
    }

    last_btn_state = current_btns;

    // 2. Analog Joystick on A6 and A7
    static uint16_t last_x = 512, last_y = 512;
    uint16_t x = adc_read(6);
    uint16_t y = adc_read(7);

    if (abs((int16_t)x - (int16_t)last_x) > 32 || abs((int16_t)y - (int16_t)last_y) > 32) {
        event_push(EVENT_JOY_MOVE, (uint8_t)(x >> 2), (uint8_t)(y >> 2));
        last_x = x;
        last_y = y;
    }
}
