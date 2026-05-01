#include <SDL2/SDL.h>
#include "../scripts/avrboy.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Logical dimensions including padding
#define LOGICAL_W (SCREEN_WIDTH + 16)
#define LOGICAL_H (SCREEN_HEIGHT + 16)

#define SAMPLE_RATE 44100

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
static void (*render_cb)(void) = NULL;

static volatile int audio_freq = 0;
static double audio_phase = 0;

const system_api_t* sim_api_ptr;

// --- Audio Callback ---
void audio_callback(void* userdata, Uint8* stream, int len) {
    int16_t* bstream = (int16_t*)stream;
    int samples = len / sizeof(int16_t);
    
    for (int i = 0; i < samples; i++) {
        if (audio_freq > 0) {
            // Generate a simple square wave
            double period = (double)SAMPLE_RATE / audio_freq;
            if (fmod(audio_phase, period) < period / 2.0) {
                bstream[i] = 4000; // Volume
            } else {
                bstream[i] = -4000;
            }
            audio_phase += 1.0;
            if (audio_phase > period * 1000) audio_phase = 0; // Prevent overflow
        } else {
            bstream[i] = 0;
            audio_phase = 0;
        }
    }
}

// --- API Implementation ---

static void sim_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        pixels[y * SCREEN_WIDTH + x] = color ? 0xFF00FF00 : 0xFF002200;
    }
}

static void sim_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
    int dx = abs((int)x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        sim_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void sim_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t i = 0; i < w; i++) {
        for (uint8_t j = 0; j < h; j++) {
            sim_draw_pixel(x + i, y + j, color);
        }
    }
}

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14}, 
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, 
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08}, 
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02}, 
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, 
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, 
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00}, 
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, 
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, 
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, 
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, 
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, 
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, 
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, 
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00}, 
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40}, 
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20}, 
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E}, 
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00}, 
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38}, 
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20}, 
    {0x04,0x3F,0x44,0x40,20}, {0x3C,0x40,0x40,20,0x7C}, {0x1C,0x20,0x40,20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C}, 
    {0x44,0x28,10,28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00}, 
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x10,0x08,0x08,0x10,0x08}
};

static void sim_draw_string(uint8_t x, uint8_t y, const char *s, uint8_t color) {
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        if (c >= 0x20 && c <= 0x7E) {
            uint8_t idx = c - 0x20;
            for (uint8_t col = 0; col < 5; col++) {
                uint8_t bits = font5x7[idx][col];
                for (uint8_t row = 0; row < 7; row++) {
                    if (bits & (1 << row)) {
                        sim_draw_pixel(x + col, y + row, color);
                    }
                }
            }
        }
        x += 6;
    }
}

static void sim_lcd_update(void) {
    // Fill backbuffer with dark green
    for(int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) pixels[i] = 0xFF002200;
    
    if (render_cb) render_cb();

    // Update texture and render
    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
    
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // Window background
    SDL_RenderClear(renderer);

    // Center the LCD in the window
    SDL_Rect dst = { 8, 8, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    
    SDL_RenderPresent(renderer);
}

static void sim_set_render_callback(void (*callback)(void)) {
    render_cb = callback;
}

static void sim_play_tone(uint16_t freq) {
    audio_freq = freq;
}

static uint8_t btn_state = 0;
static uint8_t joy_x = 128, joy_y = 128; // Center

static bool sim_poll_event(Event* e) {
    SDL_Event sev;
    while (SDL_PollEvent(&sev)) {
        if (sev.type == SDL_QUIT) exit(0);
        if (sev.type == SDL_KEYDOWN || sev.type == SDL_KEYUP) {
            uint8_t bit = 0;
            bool is_down = (sev.type == SDL_KEYDOWN);
            
            switch (sev.key.keysym.sym) {
                // Digital Buttons (Arrows + Z/X)
                case SDLK_UP:    bit = BTN_UP; break;
                case SDLK_DOWN:  bit = BTN_DOWN; break;
                case SDLK_LEFT:  bit = BTN_LEFT; break;
                case SDLK_RIGHT: bit = BTN_RIGHT; break;
                case SDLK_z:     bit = BTN_A; break;
                case SDLK_x:     bit = BTN_B; break;
                
                // Joystick Emulation (WASD)
                case SDLK_w: joy_y = is_down ? 0 : 128; break;
                case SDLK_s: joy_y = is_down ? 255 : 128; break;
                case SDLK_a: joy_x = is_down ? 0 : 128; break;
                case SDLK_d: joy_x = is_down ? 255 : 128; break;
            }

            if (bit) {
                if (is_down) btn_state |= bit;
                else btn_state &= ~bit;
                
                e->type = is_down ? EVENT_BTN_DOWN : EVENT_BTN_UP;
                e->data1 = btn_state;
                return true;
            }

            if (sev.key.keysym.sym == SDLK_w || sev.key.keysym.sym == SDLK_s || 
                sev.key.keysym.sym == SDLK_a || sev.key.keysym.sym == SDLK_d) {
                e->type = EVENT_JOY_MOVE;
                e->data1 = joy_x;
                e->data2 = joy_y;
                return true;
            }
        }
    }
    return false;
}

static uint32_t sim_get_tick(void) { return SDL_GetTicks(); }
static void sim_wait_tick(void) { SDL_Delay(16); } // ~60fps
static void sim_mpu_read(mpu_data_t* data) { memset(data, 0, sizeof(mpu_data_t)); }
static void sim_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("LOG: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

static const system_api_t simulator_api = {
    .draw_pixel = sim_draw_pixel,
    .draw_line = sim_draw_line,
    .fill_rect = sim_fill_rect,
    .draw_string = sim_draw_string,
    .lcd_update = sim_lcd_update,
    .set_render_callback = sim_set_render_callback,
    .play_tone = sim_play_tone,
    .poll_event = sim_poll_event,
    .get_tick = sim_get_tick,
    .wait_tick = sim_wait_tick,
    .mpu_read = sim_mpu_read,
    .log = sim_log
};

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;
    
    // Setup Audio Spec
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;

    if (SDL_OpenAudio(&want, &have) < 0) {
        printf("SDL_OpenAudio failed: %s\n", SDL_GetError());
    } else {
        SDL_PauseAudio(0);
    }

    window = SDL_CreateWindow("AVRboy Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              800, 450, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, LOGICAL_W, LOGICAL_H);
    
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
                                SCREEN_WIDTH, SCREEN_HEIGHT);

    sim_api_ptr = &simulator_api;

    // Call the application's main
    extern int app_main(void);
    app_main();

    SDL_CloseAudio();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
