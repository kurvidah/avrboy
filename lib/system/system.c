#include "system.h"
#include "gfx.h"
#include "lcd.h"
#include "audio.h"
#include "event.h"
#include "timer.h"
#include "uart.h"
#include "mpu6050.h"

#ifndef DEBUG
static void log_dummy(const char *fmt, ...) { (void)fmt; }
#endif

const system_api_t system_api = {
    .draw_pixel   = gfx_draw_pixel,
    .draw_line    = gfx_draw_line,
    .fill_rect    = gfx_fill_rect,
    .draw_string  = gfx_draw_string,
    .lcd_update   = lcd_update,
    .lcd_clear    = lcd_clear,

    .play_tone    = audio_play_tone,

    .poll_event   = event_poll,

    .get_tick     = timer_get_ticks,
    .wait_tick    = timer_wait_tick,

    .mpu_read     = (void(*)(void*))mpu6050_read_all,
    
#ifdef DEBUG
    .log          = uart_log
#else
    .log          = log_dummy
#endif
};
