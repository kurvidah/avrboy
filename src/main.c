#include "lcd.h"
#include "gfx.h"
#include "spi.h"
#include "adc.h"
#include "event.h"
#include "timer.h"
#include "input.h"
#include "audio.h"
#include "system.h"
#include "uart.h"
#include "mpu6050.h"
#include <avr/interrupt.h>
#include <util/delay.h>

// ── Application State (Example) ──────────────────────────────────────────────

static int8_t player_x = 64;
static int8_t player_y = 32;

void update_game_state(void) {
    Event e;
    while (system_api.poll_event(&e)) {
        if (e.type == EVENT_BTN_DOWN) {
            system_api.log("BTN DOWN: 0x%02X", e.data1);
            if (e.data1 & BTN_UP)    player_y -= 2;
            if (e.data1 & BTN_DOWN)  player_y += 2;
            if (e.data1 & BTN_LEFT)  player_x -= 2;
            if (e.data1 & BTN_RIGHT) player_x += 2;
            system_api.play_tone(440); // Play A4
            
        } else if (e.type == EVENT_BTN_UP) {
            system_api.log("BTN UP: 0x%02X", e.data1);
            system_api.play_tone(0); // Stop audio
        } else if (e.type == EVENT_JOY_MOVE) {
            system_api.log("JOY: X=%d Y=%d", e.data1, e.data2);
            // data1 is X (0-255), data2 is Y (0-255)
            // Center is ~128
            if (e.data1 < 64)  player_x -= 1;
            if (e.data1 > 192) player_x += 1;
            if (e.data2 < 64)  player_y -= 1;
            if (e.data2 > 192) player_y += 1;
        }
    }

    // Keep player in bounds
    if (player_x < 0) player_x = 0;
    if (player_x > 120) player_x = 120;
    if (player_y < 12) player_y = 12; // Below title bar
    if (player_y > 56) player_y = 56;
}

void render_frame(void) {
    system_api.lcd_clear();
    
    // Draw status bar
    system_api.fill_rect(0, 0, 128, 10, 1);
    system_api.draw_string(2, 2, "AVRBOY RUNTIME", 0);
    
    // Draw "player"
    system_api.fill_rect(player_x, player_y, 8, 8, 1);
    
    system_api.lcd_update();
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(void) {
    // 1. Initialize Hardware Drivers
    spi_init();
    lcd_init();
    uart_init(9600);
    mpu6050_init();

    // 2. Initialize Runtime Subsystems

    event_init();
    timer_init();
    input_init();
    audio_init();
    
    // 3. Enable Global Interrupts
    sei();

    system_api.log("AVRBOY System Boot @ 16MHz");

    // 4. Execution Loop (Section 4.2)
    while (1) {
        // I/O Polling (could also be in Timer ISR)
        input_poll();

        update_game_state();
        render_frame();

        // Deterministic timing (60 FPS)
        system_api.wait_tick();
    }
}