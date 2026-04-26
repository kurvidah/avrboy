#pragma once
#include "spi.h"
#include <stdint.h>

#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_PAGES  8

#define LCD_DDR   DDRB
#define LCD_PORT  PORTB
#define LCD_AO    PB0
#define LCD_RESET PB1
#define LCD_CS    PB2

#define LCD_CS_LOW()     (LCD_PORT &= ~(1 << LCD_CS))
#define LCD_CS_HIGH()    (LCD_PORT |=  (1 << LCD_CS))
#define LCD_AO_CMD()     (LCD_PORT &= ~(1 << LCD_AO))
#define LCD_AO_DATA()    (LCD_PORT |=  (1 << LCD_AO))
#define LCD_RESET_LOW()  (LCD_PORT &= ~(1 << LCD_RESET))
#define LCD_RESET_HIGH() (LCD_PORT |=  (1 << LCD_RESET))

void lcd_init(void);
void lcd_set_contrast(uint8_t contrast);

// Paging primitives
void lcd_start_page(uint8_t page);
void lcd_flush_page(void);
void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value);
