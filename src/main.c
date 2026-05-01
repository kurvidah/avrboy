#include "system.h"
#include "pff.h"
#include "spi.h"
#include "lcd.h"
#include "gfx.h"
#include "uart.h"
#include "input.h"
#include "event.h"
#include "timer.h"
#include "audio.h"
#include "mpu6050.h"
#include "loader.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

// ── System States ─────────────────────────────────────────────────────────────

typedef enum {
    STATE_BOOT,
    STATE_MENU,
    STATE_PRE_LOAD, 
    STATE_LOADING,
    STATE_RUNNING,
    STATE_ERROR
} system_state_t;

static system_state_t current_state = STATE_BOOT;
static int8_t menu_index = 0;

#define MAX_FILES 8
static char file_list[MAX_FILES][13];
static uint8_t file_count = 0;

FATFS fs;

// ── File System Helpers ───────────────────────────────────────────────────────

void scan_sd(void) {
    DIR dir;
    FILINFO fno;
    file_count = 0;

    if (pf_opendir(&fs, &dir, "/") != FR_OK) {
        current_state = STATE_ERROR;
        return;
    }

    while (file_count < MAX_FILES) {
        if (pf_readdir(&fs, &dir, &fno) != FR_OK || fno.fname[0] == 0) break;
        if (!(fno.fattrib & AM_DIR)) {
            char* dot = strrchr(fno.fname, '.');
            if (dot && strcasecmp(dot, ".HEX") == 0) {
                strncpy(file_list[file_count], fno.fname, 12);
                file_list[file_count][12] = 0;
                file_count++;
            }
        }
    }
}

void flash_program(const char* filename) {
    if (pf_open(&fs, filename) != FR_OK) {
        uart_log("OS: Open Fail");
        current_state = STATE_ERROR;
        return;
    }

    uart_log("OS: Flashing %s...", filename);
    hex_parser_init();

    char line[64];
    UINT br;
    bool processing = true;

    while (processing) {
        uint8_t i = 0;
        char c;
        while (i < 63) {
            pf_read(&fs, &c, 1, &br);
            if (br == 0) { processing = false; break; }
            if (c == '\r') continue;
            line[i++] = c;
            if (c == '\n') break;
        }
        line[i] = 0;
        if (i > 0) {
            if (!hex_parse_line(line)) processing = false;
        }
    }

    uart_log("OS: Done. Jumping...");
    _delay_ms(500);
    jump_to_app();
}

// ── Rendering ────────────────────────────────────────────────────────────────

void render_frame(void) {
    gfx_draw_string(2, 1, "AVRBOY OS v1.2", 1);
    gfx_draw_line(0, 10, 127, 10, 1);

    switch (current_state) {
        case STATE_BOOT:
            gfx_draw_string(30, 30, "MOUNTING SD...", 1);
            break;

        case STATE_MENU:
            if (file_count == 0) {
                gfx_draw_string(20, 30, "NO .HEX FILES", 1);
            } else {
                gfx_draw_string(2, 13, "SELECT CARTRIDGE:", 1);
                for (uint8_t i = 0; i < file_count; i++) {
                    uint8_t y = 25 + (i * 9);
                    if (i == menu_index) {
                        gfx_fill_rect(0, y - 1, 127, 8, 1);
                        gfx_draw_string(10, y, file_list[i], 0);
                    } else {
                        gfx_draw_string(10, y, file_list[i], 1);
                    }
                }
            }
            break;

        case STATE_PRE_LOAD:
        case STATE_LOADING:
            gfx_draw_string(25, 30, "FLASHING...", 1);
            break;

        case STATE_ERROR:
            gfx_draw_string(15, 30, "SD ERROR!", 1);
            break;

        case STATE_RUNNING:
            break;
            
        default: break;
    }
}

// ── Event Handling ────────────────────────────────────────────────────────────

void handle_events(void) {
    Event e;
    while (event_poll(&e)) {
        if (e.type == EVENT_BTN_DOWN) {
            // Re-scan SD if UP+DOWN are pressed
            if ((e.data1 & (BTN_UP | BTN_DOWN)) == (BTN_UP | BTN_DOWN)) {
                scan_sd();
                return;
            }

            if (current_state == STATE_MENU && file_count > 0) {
                if (e.data1 & BTN_UP)   menu_index = (menu_index > 0) ? menu_index - 1 : file_count - 1;
                if (e.data1 & BTN_DOWN) menu_index = (menu_index < file_count - 1) ? menu_index + 1 : 0;
                
                if (e.data1 & (BTN_A | BTN_RIGHT)) {
                    current_state = STATE_PRE_LOAD;
                    audio_play_tone(880);
                    _delay_ms(100);
                    audio_play_tone(0);
                }
            }
        } else if (e.type == EVENT_BTN_UP) {
            if (current_state == STATE_PRE_LOAD) {
                current_state = STATE_LOADING;
            }
        }
    }
}

#include <avr/pgmspace.h>

// ── Main Entry ────────────────────────────────────────────────────────────────

int main(void) {
    uart_init(9600);
    _delay_ms(100);
    uart_log("OS: START");

    // Copy System API to RAM Bridge at 0x0100
    // Standard pointers CANNOT read from Flash on AVR, so we must bridge it to RAM.
    extern const system_api_t system_api;
    for (uint8_t i = 0; i < sizeof(system_api_t); i++) {
        ((uint8_t*)0x0100)[i] = pgm_read_byte(((const uint8_t*)&system_api) + i);
    }

    spi_init();
    lcd_init();
    event_init();
    timer_init();
    input_init();
    audio_init();
    sei();

    uart_log("OS: READY");

    set_render_callback(render_frame);

    FRESULT res = pf_mount(&fs);
    if (res != FR_OK) {
        uart_log("OS: Mount Fail %d", res);
        current_state = STATE_ERROR;
    } else {
        uart_log("OS: SD Mount OK");
        scan_sd();
        current_state = STATE_MENU;
    }

    while (1) {
        handle_events();

        if (current_state == STATE_LOADING) {
            flash_program(file_list[menu_index]);
            current_state = STATE_ERROR;
        }

        system_lcd_update();
        timer_wait_tick();
    }
}
