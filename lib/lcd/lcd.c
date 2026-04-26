#include "lcd.h"
#include <avr/io.h>
#include <util/delay.h>

// ── Framebuffer ────────────────────────────────────────────────────────────────
// 128 columns × 8 pages (8 rows/page) = 1024 bytes
// buf[page][col] — bit 0 is the top row of the page, bit 7 is the bottom row.
static uint8_t framebuf[LCD_PAGES][LCD_WIDTH];

// ── Low-level helpers ──────────────────────────────────────────────────────────

static void lcd_cmd(uint8_t cmd) {
    LCD_CS_LOW();
    LCD_AO_CMD();
    spi_write(cmd);
    LCD_CS_HIGH();
}

static void lcd_data(uint8_t data) {
    LCD_CS_LOW();
    LCD_AO_DATA();
    spi_write(data);
    LCD_CS_HIGH();
}

// ── Public API ─────────────────────────────────────────────────────────────────

// In lcd_init — extend power-on delays
void lcd_init() {
    LCD_DDR |= (1 << LCD_AO) | (1 << LCD_RESET) | (1 << LCD_CS);
    LCD_CS_HIGH();

    LCD_RESET_LOW();
    _delay_ms(50);          // Longer reset pulse
    LCD_RESET_HIGH();
    _delay_ms(50);

    lcd_cmd(0xE2);          // Software reset
    _delay_ms(10);

    lcd_cmd(0x2C); _delay_ms(50);  // Booster
    lcd_cmd(0x2E); _delay_ms(50);  // Regulator
    lcd_cmd(0x2F); _delay_ms(50);  // Follower

    lcd_cmd(0x23);          // Resistor ratio
    lcd_cmd(0x81);          // Contrast command
    lcd_cmd(0x28);          // Contrast value

    lcd_cmd(0xA2);          // 1/9 bias
    lcd_cmd(0xA1);          // Seg normal
    lcd_cmd(0xC8);          // COM reverse

    lcd_cmd(0xA4);          // Normal display
    lcd_cmd(0xA6);          // Non-inverted

    lcd_cmd(0x40);          // Start line 0

    lcd_cmd(0xAF);          // Display ON
    _delay_ms(100);         // Let display stabilise before first write

    lcd_clear();
    lcd_update();
}

void lcd_clear() {
    memset(framebuf, 0x00, sizeof(framebuf));
}

void lcd_fill(uint8_t value) {
    memset(framebuf, value, sizeof(framebuf));
}

void lcd_update() {
    for (uint8_t page = 0; page < LCD_PAGES; page++) {
        lcd_cmd(0xB0 | page);
        lcd_cmd(0x10 | (4 >> 4));    // column high nibble = 0
        lcd_cmd(0x00 | (4 & 0x0F)); // column low nibble = 4
        for (uint8_t col = 0; col < LCD_WIDTH; col++) {
            lcd_data(framebuf[page][col]);
        }
    }
}

void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;

    if (value) {
        framebuf[page][x] |=  (1 << bit);
    } else {
        framebuf[page][x] &= ~(1 << bit);
    }
}

uint8_t lcd_get_pixel(uint8_t x, uint8_t y) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return 0;

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;

    return (framebuf[page][x] >> bit) & 0x01;
}

void lcd_set_contrast(uint8_t contrast) {
    if (contrast > 63) contrast = 63;
    lcd_cmd(0x81);
    lcd_cmd(contrast);
}

void lcd_set_display_on(uint8_t on) {
    lcd_cmd(on ? 0xAF : 0xAE);
}