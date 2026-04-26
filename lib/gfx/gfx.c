#include "gfx.h"
#include <stdlib.h>
#include <avr/pgmspace.h>

// ── Internal helpers ──────────────────────────────────────────────────────────

static inline int16_t gfx_abs(int16_t v) { return v < 0 ? -v : v; }
static inline int16_t gfx_min(int16_t a, int16_t b) { return a < b ? a : b; }
static inline int16_t gfx_max(int16_t a, int16_t b) { return a > b ? a : b; }

static inline void _px(int16_t x, int16_t y, uint8_t value) {
    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT)
        lcd_set_pixel((uint8_t)x, (uint8_t)y, value);
}

// ── Pixel ─────────────────────────────────────────────────────────────────────

void gfx_draw_pixel(uint8_t x, uint8_t y, uint8_t value) {
    lcd_set_pixel(x, y, value);
}

// ── Lines ─────────────────────────────────────────────────────────────────────

void gfx_draw_hline(uint8_t x, uint8_t y, uint8_t w, uint8_t value) {
    for (uint8_t i = 0; i < w; i++) _px(x + i, y, value);
}

void gfx_draw_vline(uint8_t x, uint8_t y, uint8_t h, uint8_t value) {
    for (uint8_t i = 0; i < h; i++) _px(x, y + i, value);
}

// Bresenham's line algorithm
void gfx_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t value) {
    int16_t dx =  gfx_abs((int16_t)x1 - x0);
    int16_t dy = -gfx_abs((int16_t)y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t cx = x0, cy = y0;

    while (1) {
        _px(cx, cy, value);
        if (cx == x1 && cy == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
}

// ── Rectangles ────────────────────────────────────────────────────────────────

void gfx_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t value) {
    gfx_draw_hline(x,         y,         w, value);
    gfx_draw_hline(x,         y + h - 1, w, value);
    gfx_draw_vline(x,         y,         h, value);
    gfx_draw_vline(x + w - 1, y,         h, value);
}

void gfx_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t value) {
    for (uint8_t row = 0; row < h; row++)
        gfx_draw_hline(x, y + row, w, value);
}

// Midpoint circle algorithm — draws only the four corner arcs used by rounded rects
static void _round_corners(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                            uint8_t r, uint8_t value, uint8_t fill) {
    int16_t f     = 1 - r;
    int16_t ddx   = 1;
    int16_t ddy   = -2 * r;
    int16_t px    = 0;
    int16_t py    = r;

    // Corner centres
    uint8_t x0 = x + r;
    uint8_t y0 = y + r;
    uint8_t x1 = x + w - 1 - r;
    uint8_t y1 = y + h - 1 - r;

    while (px <= py) {
        if (fill) {
            // Fill: draw horizontal spans connecting opposing corners
            gfx_draw_hline(x0 - px, y0 - py, (x1 - x0) + 2 * px + 1, value);
            gfx_draw_hline(x0 - px, y1 + py, (x1 - x0) + 2 * px + 1, value);
            gfx_draw_hline(x0 - py, y0 - px, (x1 - x0) + 2 * py + 1, value);
            gfx_draw_hline(x0 - py, y1 + px, (x1 - x0) + 2 * py + 1, value);
        } else {
            // Outline: draw 8 arc pixels (2 per quadrant)
            _px(x1 + px, y0 - py, value); _px(x0 - px, y0 - py, value);
            _px(x1 + px, y1 + py, value); _px(x0 - px, y1 + py, value);
            _px(x1 + py, y0 - px, value); _px(x0 - py, y0 - px, value);
            _px(x1 + py, y1 + px, value); _px(x0 - py, y1 + px, value);
        }

        if (f >= 0) { py--; ddy += 2; f += ddy; }
        px++; ddx += 2; f += ddx;
    }
}

void gfx_draw_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                          uint8_t r, uint8_t value) {
    uint8_t inner = w - 2 * r;
    gfx_draw_hline(x + r, y,         inner, value);
    gfx_draw_hline(x + r, y + h - 1, inner, value);
    gfx_draw_vline(x,         y + r, h - 2 * r, value);
    gfx_draw_vline(x + w - 1, y + r, h - 2 * r, value);
    _round_corners(x, y, w, h, r, value, 0);
}

void gfx_fill_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                          uint8_t r, uint8_t value) {
    gfx_fill_rect(x + r, y, w - 2 * r, h, value);
    _round_corners(x, y, w, h, r, value, 1);
}

// ── Circles ───────────────────────────────────────────────────────────────────

static void _circle_points(int16_t cx, int16_t cy,
                            int16_t px, int16_t py, uint8_t value) {
    _px(cx + px, cy + py, value); _px(cx - px, cy + py, value);
    _px(cx + px, cy - py, value); _px(cx - px, cy - py, value);
    _px(cx + py, cy + px, value); _px(cx - py, cy + px, value);
    _px(cx + py, cy - px, value); _px(cx - py, cy - px, value);
}

void gfx_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t value) {
    int16_t f  = 1 - r, ddx = 1, ddy = -2 * r, px = 0, py = r;
    _px(cx, cy + r, value); _px(cx, cy - r, value);
    _px(cx + r, cy, value); _px(cx - r, cy, value);
    while (px < py) {
        if (f >= 0) { py--; ddy += 2; f += ddy; }
        px++; ddx += 2; f += ddx;
        _circle_points(cx, cy, px, py, value);
    }
}

void gfx_fill_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t value) {
    int16_t f  = 1 - r, ddx = 1, ddy = -2 * r, px = 0, py = r;
    gfx_draw_vline(cx, cy - r, 2 * r + 1, value);
    while (px < py) {
        if (f >= 0) { py--; ddy += 2; f += ddy; }
        px++; ddx += 2; f += ddx;
        gfx_draw_vline(cx + px, cy - py, 2 * py + 1, value);
        gfx_draw_vline(cx - px, cy - py, 2 * py + 1, value);
        gfx_draw_vline(cx + py, cy - px, 2 * px + 1, value);
        gfx_draw_vline(cx - py, cy - px, 2 * px + 1, value);
    }
}

// ── Triangles ─────────────────────────────────────────────────────────────────

void gfx_draw_triangle(uint8_t x0, uint8_t y0,
                        uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2, uint8_t value) {
    gfx_draw_line(x0, y0, x1, y1, value);
    gfx_draw_line(x1, y1, x2, y2, value);
    gfx_draw_line(x2, y2, x0, y0, value);
}

// Flat-bottom / flat-top scanline fill
static void _fill_flat_bottom(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2, uint8_t value) {
    // y1 == y2 (flat bottom)
    float slope1 = (float)(x1 - x0) / (y1 - y0);
    float slope2 = (float)(x2 - x0) / (y2 - y0);
    float xa = x0, xb = x0;
    for (int16_t y = y0; y <= y1; y++) {
        int16_t lx = (int16_t)gfx_min((int16_t)xa, (int16_t)xb);
        int16_t rx = (int16_t)gfx_max((int16_t)xa, (int16_t)xb);
        if (lx < LCD_WIDTH && rx >= 0 && y >= 0 && y < LCD_HEIGHT)
            gfx_draw_hline((uint8_t)gfx_max(lx, 0), (uint8_t)y,
                           (uint8_t)(gfx_min(rx, LCD_WIDTH - 1) - gfx_max(lx, 0) + 1), value);
        xa += slope1; xb += slope2;
    }
}

static void _fill_flat_top(int16_t x0, int16_t y0,
                            int16_t x1, int16_t y1,
                            int16_t x2, int16_t y2, uint8_t value) {
    // y0 == y1 (flat top)
    float slope1 = (float)(x2 - x0) / (y2 - y0);
    float slope2 = (float)(x2 - x1) / (y2 - y1);
    float xa = x2, xb = x2;
    for (int16_t y = y2; y >= y0; y--) {
        int16_t lx = (int16_t)gfx_min((int16_t)xa, (int16_t)xb);
        int16_t rx = (int16_t)gfx_max((int16_t)xa, (int16_t)xb);
        if (lx < LCD_WIDTH && rx >= 0 && y >= 0 && y < LCD_HEIGHT)
            gfx_draw_hline((uint8_t)gfx_max(lx, 0), (uint8_t)y,
                           (uint8_t)(gfx_min(rx, LCD_WIDTH - 1) - gfx_max(lx, 0) + 1), value);
        xa -= slope1; xb -= slope2;
    }
}

void gfx_fill_triangle(uint8_t x0, uint8_t y0,
                        uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2, uint8_t value) {
    // Sort vertices by Y: v0 top, v2 bottom
    if (y0 > y1) { uint8_t t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
    if (y1 > y2) { uint8_t t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }
    if (y0 > y1) { uint8_t t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }

    if (y1 == y2) {
        _fill_flat_bottom(x0,y0, x1,y1, x2,y2, value);
    } else if (y0 == y1) {
        _fill_flat_top(x0,y0, x1,y1, x2,y2, value);
    } else {
        // Split into flat-bottom + flat-top at the midpoint
        uint8_t mx = (uint8_t)(x0 + (float)(y1 - y0) / (y2 - y0) * (x2 - x0));
        uint8_t my = y1;
        _fill_flat_bottom(x0,y0, x1,y1, mx,my, value);
        _fill_flat_top   (x1,y1, mx,my, x2,y2, value);
    }
}

// ── Font (5x7 ASCII 0x20–0x7E) ────────────────────────────────────────────────

static const uint8_t font5x7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x10,0x08,0x08,0x10,0x08}, // '~'
};

#define FONT_CHAR_W  5
#define FONT_CHAR_H  7
#define FONT_ADVANCE 6   // 5px glyph + 1px gap

void gfx_draw_char(uint8_t x, uint8_t y, char c, uint8_t value) {
    if (c < 0x20 || c > 0x7E) return;
    for (uint8_t col = 0; col < FONT_CHAR_W; col++) {
        uint8_t bits = pgm_read_byte(&font5x7[(uint8_t)(c - 0x20)][col]);
        for (uint8_t row = 0; row < FONT_CHAR_H; row++) {
            if (bits & (1 << row))
                _px(x + col, y + row, value);
        }
    }
}

void gfx_draw_string(uint8_t x, uint8_t y, const char *s, uint8_t value) {
    while (*s) {
        if (x + FONT_CHAR_W > LCD_WIDTH) break;
        gfx_draw_char(x, y, *s++, value);
        x += FONT_ADVANCE;
    }
}

// ── Bitmap ────────────────────────────────────────────────────────────────────

void gfx_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                     const uint8_t *bits, uint8_t value) {
    uint8_t stride = (w + 7) / 8;   // bytes per row
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            uint8_t byte_idx = col / 8;
            uint8_t bit_idx  = 7 - (col % 8);  // MSB first
            if (bits[row * stride + byte_idx] & (1 << bit_idx))
                _px(x + col, y + row, value);
        }
    }
}