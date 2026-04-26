#pragma once

#include "event.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // Video (Transparent Paging)
    void (*draw_pixel)(uint8_t x, uint8_t y, uint8_t color);
    void (*draw_line)(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
    void (*fill_rect)(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
    void (*draw_string)(uint8_t x, uint8_t y, const char *s, uint8_t color);
    void (*lcd_update)(void);

    // Runtime paging configuration
    void (*set_render_callback)(void (*callback)(void));

    // Audio
    void (*play_tone)(uint16_t freq);

    // Input / Events
    bool (*poll_event)(Event* e);

    // Timing
    uint32_t (*get_tick)(void);
    void (*wait_tick)(void);

    // MPU6050
    void (*mpu_read)(void* data);

    // Debugging
    void (*log)(const char *fmt, ...);
} system_api_t;

extern const system_api_t system_api;
