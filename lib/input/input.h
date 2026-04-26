#pragma once

#include <stdint.h>

// Buttons moved to PORTC (A0-A5) to be adjacent and avoid PD3 conflict
#define INPUT_PORT PORTC
#define INPUT_PIN  PINC
#define INPUT_DDR  DDRC

#define MASK_UP    (1 << 0) // A0
#define MASK_DOWN  (1 << 1) // A1
#define MASK_LEFT  (1 << 2) // A2
#define MASK_RIGHT (1 << 3) // A3
#define MASK_A     (1 << 4) // A4
#define MASK_B     (1 << 5) // A5

// Initialize input pins and ADC for joystick
void input_init(void);

// Poll inputs and push events to the queue
void input_poll(void);

