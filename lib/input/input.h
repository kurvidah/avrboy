#pragma once

#include <stdint.h>

// Port C Buttons
#define PORTC_MASK_UP    (1 << 2) // PC2
#define PORTC_MASK_DOWN  (1 << 3) // PC3

// Port D Buttons
#define PORTD_MASK_LEFT  (1 << 2) // PD2
#define PORTD_MASK_RIGHT (1 << 5) // PD5
#define PORTD_MASK_A     (1 << 6) // PD6
#define PORTD_MASK_B     (1 << 7) // PD7

// Combined Logical Bitmask (for the system_api events)
#define BTN_UP    (1 << 0)
#define BTN_DOWN  (1 << 1)
#define BTN_LEFT  (1 << 2)
#define BTN_RIGHT (1 << 3)
#define BTN_A     (1 << 4)
#define BTN_B     (1 << 5)

void input_init(void);
void input_poll(void);
