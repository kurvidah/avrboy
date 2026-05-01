#include <SDL2/SDL.h>
#include "../scripts/avrboy.h"
#include <stdio.h>
#include <stdarg.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCALE 4

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static uint8_t screen_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static void (*render_cb)(void) = NULL;

const system_api_t* sim_api_ptr;

// --- API Implementation ---

static void sim_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        screen_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

static void sim_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
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

static void sim_draw_string(uint8_t x, uint8_t y, const char *s, uint8_t color) {
    // Very simple 5x7 font-like drawing (placeholder)
    while (*s) {
        sim_fill_rect(x, y, 4, 6, color); // Just draw blocks for now
        x += 6;
        s++;
    }
}

static void sim_lcd_update(void) {
    if (render_cb) render_cb();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (screen_buffer[x + y * SCREEN_WIDTH]) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 20, 0, 255);
            }
            SDL_Rect r = { x * SCALE, y * SCALE, SCALE, SCALE };
            SDL_RenderFillRect(renderer, &r);
        }
    }
    SDL_RenderPresent(renderer);
}

static void sim_set_render_callback(void (*callback)(void)) {
    render_cb = callback;
}

static void sim_play_tone(uint16_t freq) {
    printf("AUDIO: %d Hz\n", freq);
}

static uint8_t btn_state = 0;

static bool sim_poll_event(Event* e) {
    SDL_Event sev;
    while (SDL_PollEvent(&sev)) {
        if (sev.type == SDL_QUIT) exit(0);
        if (sev.type == SDL_KEYDOWN || sev.type == SDL_KEYUP) {
            uint8_t bit = 0;
            switch (sev.key.keysym.sym) {
                case SDLK_UP:    bit = BTN_UP; break;
                case SDLK_DOWN:  bit = BTN_DOWN; break;
                case SDLK_LEFT:  bit = BTN_LEFT; break;
                case SDLK_RIGHT: bit = BTN_RIGHT; break;
                case SDLK_z:     bit = BTN_A; break;
                case SDLK_x:     bit = BTN_B; break;
            }
            if (bit) {
                if (sev.type == SDL_KEYDOWN) btn_state |= bit;
                else btn_state &= ~bit;
                
                e->type = (sev.type == SDL_KEYDOWN) ? EVENT_BTN_DOWN : EVENT_BTN_UP;
                e->data1 = btn_state;
                return true;
            }
        }
    }
    return false;
}

static uint32_t sim_get_tick(void) { return SDL_GetTicks(); }
static void sim_wait_tick(void) { SDL_Delay(16); } // ~60fps
static void sim_mpu_read(void* data) { memset(data, 0, 14); }
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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    window = SDL_CreateWindow("AVRboy Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    sim_api_ptr = &simulator_api;

    // Call the application's main
    extern int app_main(void);
    app_main();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
