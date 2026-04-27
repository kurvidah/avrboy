#include "system.h"
#include "spi.h"
#include "lcd.h"
#include "uart.h"
#include "input.h"
#include "event.h"
#include "timer.h"
#include "audio.h"
#include "mpu6050.h"
#include "pff.h"
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
        system_api.log("OS: Open Fail");
        current_state = STATE_ERROR;
        return;
    }

    system_api.log("OS: Flashing %s...", filename);
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

    system_api.log("OS: Done. Jumping...");
    _delay_ms(500);
    jump_to_app();
}

// ── Rendering ────────────────────────────────────────────────────────────────

void render_frame(void) {
    system_api.draw_string(2, 1, "AVRBOY OS v1.2", 1);
    system_api.draw_line(0, 10, 127, 10, 1);

    switch (current_state) {
        case STATE_BOOT:
            system_api.draw_string(30, 30, "MOUNTING SD...", 1);
            break;

        case STATE_MENU:
            if (file_count == 0) {
                system_api.draw_string(20, 30, "NO .HEX FILES", 1);
            } else {
                system_api.draw_string(2, 13, "SELECT CARTRIDGE:", 1);
                for (uint8_t i = 0; i < file_count; i++) {
                    uint8_t y = 25 + (i * 9);
                    if (i == menu_index) {
                        system_api.fill_rect(0, y - 1, 127, 8, 1);
                        system_api.draw_string(10, y, file_list[i], 0);
                    } else {
                        system_api.draw_string(10, y, file_list[i], 1);
                    }
                }
            }
            break;

        case STATE_PRE_LOAD:
        case STATE_LOADING:
            system_api.draw_string(25, 30, "FLASHING...", 1);
            break;

        case STATE_ERROR:
            system_api.draw_string(15, 30, "SD ERROR!", 1);
            break;

        case STATE_RUNNING:
            break;
            
        default: break;
    }
}

// ── Event Handling ────────────────────────────────────────────────────────────

void handle_events(void) {
    Event e;
    while (system_api.poll_event(&e)) {
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
                    system_api.play_tone(880);
                    _delay_ms(100);
                    system_api.play_tone(0);
                }
            }
        } else if (e.type == EVENT_BTN_UP) {
            if (current_state == STATE_PRE_LOAD) {
                current_state = STATE_LOADING;
            }
        }
    }
}

// ── Main Entry ────────────────────────────────────────────────────────────────

int main(void) {
    spi_init();
    lcd_init();
    uart_init(9600);
    event_init();
    timer_init();
    input_init();
    audio_init();
    sei();

    system_api.set_render_callback(render_frame);

    FRESULT res = pf_mount(&fs);
    if (res != FR_OK) {
        system_api.log("OS: Mount Fail %d", res);
        current_state = STATE_ERROR;
    } else {
        system_api.log("OS: SD Mount OK");
        scan_sd();
        current_state = STATE_MENU;
    }

    while (1) {
        input_poll();
        handle_events();

        if (current_state == STATE_LOADING) {
            flash_program(file_list[menu_index]);
            current_state = STATE_ERROR;
        }

        system_api.lcd_update();
        system_api.wait_tick();
    }
}
