#ifndef AVRBOY_H
#define AVRBOY_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/* --- System Structures --- */

typedef struct {
    uint8_t type;
    uint8_t data1;
    uint8_t data2;
} Event;

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t temp;
} mpu_data_t;

typedef struct {
    void (*draw_pixel)(uint8_t x, uint8_t y, uint8_t color);
    void (*draw_line)(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
    void (*fill_rect)(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
    void (*draw_string)(uint8_t x, uint8_t y, const char *s, uint8_t color);
    void (*lcd_update)(void);
    void (*set_render_callback)(void (*callback)(void));
    void (*play_tone)(uint16_t freq);
    bool (*poll_event)(Event* e);
    uint32_t (*get_tick)(void);
    void (*wait_tick)(void);
    void (*mpu_read)(mpu_data_t* data);
    void (*log)(const char *fmt, ...);
} system_api_t;

/* --- API Bridge --- */

#ifdef SIMULATOR
    extern const system_api_t* sim_api_ptr;
    #define system_api (*sim_api_ptr)
    #define APP_MAIN() int app_main(void)
#else
    /* OS Bridge at 0x0100 */
    #define system_api (*((const system_api_t*)0x0100))
    #define APP_MAIN() \
        int avrboy_main(void); \
        int main(void) { \
            sei(); \
            return avrboy_main(); \
        } \
        int avrboy_main(void)
#endif

/* --- Constants --- */

#define BTN_UP    (1 << 0)
#define BTN_DOWN  (1 << 1)
#define BTN_LEFT  (1 << 2)
#define BTN_RIGHT (1 << 3)
#define BTN_A     (1 << 4)
#define BTN_B     (1 << 5)

#define EVENT_BTN_DOWN 1
#define EVENT_BTN_UP   2
#define EVENT_JOY_MOVE 3
#define EVENT_TICK     4

#endif // AVRBOY_H
