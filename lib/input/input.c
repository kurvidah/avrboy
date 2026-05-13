#include "input.h"
#include "event.h"
#include "adc.h"
#include <avr/io.h>
#include <stdlib.h>

static uint8_t last_btn_state = 0;
static uint16_t joy_center_x = 512;
static uint16_t joy_center_y = 512;

void input_init(void) {
    // 1. Setup PORTC Buttons (PC2, PC3)
    DDRC  &= ~(PORTC_MASK_UP | PORTC_MASK_DOWN);
    PORTC |=  (PORTC_MASK_UP | PORTC_MASK_DOWN); // Pull-ups

    // 2. Setup PORTD Buttons (PD2, PD5, PD6, PD7)
    DDRD  &= ~(PORTD_MASK_LEFT | PORTD_MASK_RIGHT | PORTD_MASK_A | PORTD_MASK_B);
    PORTD |=  (PORTD_MASK_LEFT | PORTD_MASK_RIGHT | PORTD_MASK_A | PORTD_MASK_B); // Pull-ups
    
    // 3. ADC initialized for channels 0 and 1 (Joystick X/Y)
    adc_init(0b00000011); 

    // 4. Auto-Calibration: Sample idle position
    uint32_t sx = 0, sy = 0;
    for(uint8_t i=0; i<8; i++) {
        sx += adc_read(0);
        sy += adc_read(1);
    }
    joy_center_x = sx / 8;
    joy_center_y = sy / 8;
    // joy_center_x = 512;
    // joy_center_y = 512;
}

void input_poll(void) {
    // --- 1. Gather Physical States ---
    uint8_t current_btns = 0;
    uint8_t pinc = PINC;
    if (!(pinc & PORTC_MASK_UP))   current_btns |= BTN_UP;
    if (!(pinc & PORTC_MASK_DOWN)) current_btns |= BTN_DOWN;

    uint8_t pind = PIND;
    if (!(pind & PORTD_MASK_LEFT))  current_btns |= BTN_LEFT;
    if (!(pind & PORTD_MASK_RIGHT)) current_btns |= BTN_RIGHT;
    if (!(pind & PORTD_MASK_A))     current_btns |= BTN_A;
    if (!(pind & PORTD_MASK_B))     current_btns |= BTN_B;

    // --- 2. Event Generation ---
    uint8_t changed_down = current_btns & ~last_btn_state;
    if (changed_down) event_push(EVENT_BTN_DOWN, current_btns, 0);

    uint8_t changed_up = ~current_btns & last_btn_state;
    if (changed_up) event_push(EVENT_BTN_UP, current_btns, 0);

    last_btn_state = current_btns;

    // --- 3. Analog Joystick (Calibrated) ---
    static uint8_t adc_counter = 0;
    if (++adc_counter < 4) return;
    adc_counter = 0;

    static uint8_t last_jx = 128, last_jy = 128;
    
    // Read raw
    uint16_t rx = adc_read(0);
    uint16_t ry = adc_read(1);

    // Apply calibration and map to 0-255
    // Logic: center is 128. values < center map to 0-128, values > center map 128-255
    int32_t val_x, val_y;
    
    if (rx < joy_center_x) val_x = (int32_t)rx * 128 / joy_center_x;
    else val_x = 128 + ((int32_t)(rx - joy_center_x) * 127 / (1023 - joy_center_x));

    if (ry < joy_center_y) val_y = (int32_t)ry * 128 / joy_center_y;
    else val_y = 128 + ((int32_t)(ry - joy_center_y) * 127 / (1023 - joy_center_y));

    // Invert X/Y to match standard orientation if needed (1023 - raw)
    uint8_t final_x = 255 - (uint8_t)val_x;
    uint8_t final_y = 255 - (uint8_t)val_y;

    if (abs((int16_t)final_x - (int16_t)last_jx) > 4 || abs((int16_t)final_y - (int16_t)last_jy) > 4) {
        event_push(EVENT_JOY_MOVE, final_x, final_y);
        last_jx = final_x;
        last_jy = final_y;
    }
}
