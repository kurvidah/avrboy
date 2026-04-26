#pragma once

#include <stdint.h>

// Initialize Timer 2 for PWM audio output on OC2B (PD3)
void audio_init(void);

// Play a tone with the specified frequency (in Hz)
// freq = 0 stops the audio
void audio_play_tone(uint16_t freq);

