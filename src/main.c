#include "system.h"
#include "spi.h"
#include "lcd.h"
#include "uart.h"
#include "input.h"
#include "event.h"
#include "timer.h"
#include "audio.h"
#include <avr/interrupt.h>
#include <util/delay.h>

// ── System States ─────────────────────────────────────────────────────────────

typedef enum {
    STATE_BOOT,
    STATE_MENU,
    STATE_LOADING,
    STATE_RUNNING,
    STATE_ERROR
} system_state_t;

static system_state_t current_state = STATE_BOOT;
static int8_t menu_index = 0;
const char* const menu_items[] = { "SNAKE", "BREAKOUT", "INVADERS", "SETTINGS" };
#define MENU_COUNT 4

// ── Rendering (Called 8x per frame by paging system) ─────────────────────────

void render_frame(void) {
    // Shared Header
    system_api.draw_string(2, 1, "AVRBOY OS v1.0", 1);
    system_api.draw_line(0, 10, 127, 10, 1);

    switch (current_state) {
        case STATE_BOOT:
            system_api.draw_string(30, 30, "BOOTING...", 1);
            break;

        case STATE_MENU:
            system_api.draw_string(2, 13, "SELECT PROGRAM:", 1);
            for (uint8_t i = 0; i < MENU_COUNT; i++) {
                uint8_t y = 25 + (i * 9);
                if (i == menu_index) {
                    system_api.fill_rect(0, y - 1, 127, 8, 1);
                    system_api.draw_string(10, y, menu_items[i], 0);
                } else {
                    system_api.draw_string(10, y, menu_items[i], 1);
                }
            }
            break;

        case STATE_LOADING:
            system_api.draw_string(25, 30, "FLASHING...", 1);
            system_api.draw_line(20, 42, 108, 42, 1);
            system_api.draw_line(20, 52, 108, 52, 1);
            break;

        case STATE_RUNNING:
            system_api.draw_string(35, 30, "RUNNING", 1);
            break;

        case STATE_ERROR:
            system_api.draw_string(15, 30, "SD CARD ERROR!", 1);
            break;
    }
}

// ── Event Handling ────────────────────────────────────────────────────────────

void handle_events(void) {
    Event e;
    while (system_api.poll_event(&e)) {
        if (e.type == EVENT_BTN_DOWN) {
            if (current_state == STATE_MENU) {
                if (e.data1 & BTN_UP)   menu_index = (menu_index > 0) ? menu_index - 1 : MENU_COUNT - 1;
                if (e.data1 & BTN_DOWN) menu_index = (menu_index < MENU_COUNT - 1) ? menu_index + 1 : 0;
                
                if (e.data1 & (BTN_A | BTN_RIGHT)) {
                    system_api.log("Selected: %s", menu_items[menu_index]);
                    current_state = STATE_LOADING;
                    system_api.play_tone(880);
                }
            }
        }
    }
}

// ── Main Entry ────────────────────────────────────────────────────────────────

int main(void) {
    // 1. Hardware Init
    spi_init();
    lcd_init();
    uart_init(9600);
    event_init();
    timer_init();
    input_init();
    audio_init();
    sei();

    // 2. Runtime Configuration
    system_api.set_render_callback(render_frame);
    system_api.log("OS: Ready");

    // 3. Initial Boot sequence
    _delay_ms(500);
    current_state = STATE_MENU;

    // 4. Main Loop
    while (1) {
        input_poll();
        handle_events();

        if (current_state == STATE_LOADING) {
            // Placeholder for SD Flashing logic
            _delay_ms(1500); 
            current_state = STATE_RUNNING;
            system_api.play_tone(0);
        }

        system_api.lcd_update();
        system_api.wait_tick();
    }
}
