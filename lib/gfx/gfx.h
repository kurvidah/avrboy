#pragma once
#include "lcd.h"

// ── Primitives ────────────────────────────────────────────────────────────────
void gfx_draw_pixel   (uint8_t x, uint8_t y, uint8_t value);

void gfx_draw_line    (uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t value);
void gfx_draw_hline   (uint8_t x,  uint8_t y,  uint8_t w, uint8_t value);
void gfx_draw_vline   (uint8_t x,  uint8_t y,  uint8_t h, uint8_t value);

// ── Rectangles ────────────────────────────────────────────────────────────────
void gfx_draw_rect    (uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t value);
void gfx_fill_rect    (uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t value);
void gfx_draw_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t value);
void gfx_fill_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t value);

// ── Circles ───────────────────────────────────────────────────────────────────
void gfx_draw_circle  (uint8_t cx, uint8_t cy, uint8_t r, uint8_t value);
void gfx_fill_circle  (uint8_t cx, uint8_t cy, uint8_t r, uint8_t value);

// ── Triangles ─────────────────────────────────────────────────────────────────
void gfx_draw_triangle(uint8_t x0, uint8_t y0,
                        uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2, uint8_t value);
void gfx_fill_triangle(uint8_t x0, uint8_t y0,
                        uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2, uint8_t value);

// ── Text ──────────────────────────────────────────────────────────────────────
// Built-in 5x7 font. Each char is 5 cols wide + 1 col gap = 6px advance.
void gfx_draw_char    (uint8_t x, uint8_t y, char c,        uint8_t value);
void gfx_draw_string  (uint8_t x, uint8_t y, const char *s, uint8_t value);

// ── Bitmap ────────────────────────────────────────────────────────────────────
// bits: row-major, MSB first. 1 = draw, 0 = skip (transparent).
void gfx_draw_bitmap  (uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                        const uint8_t *bits, uint8_t value);