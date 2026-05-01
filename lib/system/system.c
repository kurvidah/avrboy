#include "system.h"
#include "audio.h"
#include "lcd.h"
#include "gfx.h"
#include "event.h"
#include "timer.h"
#include "uart.h"
#include "mpu6050.h"
#include <stdarg.h>
#include <stdio.h>

static void (*render_cb)(void) = NULL;

static void set_render_callback(void (*callback)(void)) {
    render_cb = callback;
}

static void system_lcd_update(void) {
    if (!render_cb) return;

    for (uint8_t p = 0; p < LCD_PAGES; p++) {
        lcd_start_page(p);
        render_cb();
        lcd_flush_page();
    }
}

#ifndef DEBUG
static void log_dummy(const char *fmt, ...) { (void)fmt; }
#endif

const system_api_t* const system_api_ptr __attribute__((section(".system_api"))) = &system_api;

const system_api_t system_api __attribute__((section(".system_api"))) = {
    .draw_pixel = gfx_draw_pixel,
    .draw_line = gfx_draw_line,
    .fill_rect = gfx_fill_rect,
    .draw_string = gfx_draw_string,
    .lcd_update = system_lcd_update,
    .set_render_callback = set_render_callback,
    .play_tone = audio_play_tone,
    .poll_event = event_poll,
    .get_tick = timer_get_ticks,
    .wait_tick = timer_wait_tick,
    .mpu_read = (void (*)(void*))mpu6050_read_all,
#ifdef DEBUG
    .log          = uart_log
#else
    .log          = log_dummy
#endif
};
