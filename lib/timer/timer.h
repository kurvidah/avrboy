#pragma once

#include <stdint.h>

// Initialize Timer 1 for 60Hz system tick
void timer_init(void);

// Returns the total number of system ticks since startup
uint32_t timer_get_ticks(void);

// Wait until the next system tick (frame synchronization)
void timer_wait_tick(void);

