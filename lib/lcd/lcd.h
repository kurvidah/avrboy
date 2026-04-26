#pragma once
#include "spi.h"
#include <string.h>

#define LCD_DDR  DDRB
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_PAGES  (LCD_HEIGHT / 8)   // 8 pages of 8 rows each

#define LCD_AO    PB0   // Data/Command select (A0)
#define LCD_RESET PB1   // Reset
#define LCD_CS    PB2   // Chip Select

#define LCD_PORT PORTB

#define LCD_CS_LOW()     (LCD_PORT &= ~(1 << LCD_CS))
#define LCD_CS_HIGH()    (LCD_PORT |=  (1 << LCD_CS))
#define LCD_AO_CMD()     (LCD_PORT &= ~(1 << LCD_AO))
#define LCD_AO_DATA()    (LCD_PORT |=  (1 << LCD_AO))
#define LCD_RESET_LOW()  (LCD_PORT &= ~(1 << LCD_RESET))
#define LCD_RESET_HIGH() (LCD_PORT |=  (1 << LCD_RESET))
// In lcd_init — extend power-on delays
void lcd_init();

void lcd_clear();

void lcd_fill(uint8_t value);

void lcd_update();

void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value);

uint8_t lcd_get_pixel(uint8_t x, uint8_t y);

void lcd_set_contrast(uint8_t contrast);

void lcd_set_display_on(uint8_t on);