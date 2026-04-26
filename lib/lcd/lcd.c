#include "lcd.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

static uint8_t page_buffer[LCD_WIDTH];
static uint8_t active_page = 0;

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

void lcd_init(void) {
    LCD_DDR |= (1 << LCD_AO) | (1 << LCD_RESET) | (1 << LCD_CS);
    LCD_CS_HIGH();
    LCD_RESET_LOW();
    _delay_ms(50);
    LCD_RESET_HIGH();
    _delay_ms(50);
    lcd_cmd(0xE2);
    _delay_ms(10);
    lcd_cmd(0x2C); _delay_ms(50);
    lcd_cmd(0x2E); _delay_ms(50);
    lcd_cmd(0x2F); _delay_ms(50);
    lcd_cmd(0x23);
    lcd_cmd(0x81);
    lcd_cmd(0x28);
    lcd_cmd(0xA2);
    lcd_cmd(0xA1);
    lcd_cmd(0xC8);
    lcd_cmd(0xA4);
    lcd_cmd(0xA6);
    lcd_cmd(0x40);
    lcd_cmd(0xAF);
    _delay_ms(100);
}

void lcd_start_page(uint8_t page) {
    active_page = page;
    memset(page_buffer, 0, sizeof(page_buffer));
}

void lcd_flush_page(void) {
    uint8_t real_col = 4; // Hardware offset
    lcd_cmd(0xB0 | active_page);
    lcd_cmd(0x10 | (real_col >> 4));
    lcd_cmd(0x00 | (real_col & 0x0F));
    for (uint8_t col = 0; col < LCD_WIDTH; col++) {
        lcd_data(page_buffer[col]);
    }
}

void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t value) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // Transparent Paging: Only write if Y falls within current page
    if ((y >> 3) == active_page) {
        if (value) page_buffer[x] |=  (1 << (y & 0x07));
        else       page_buffer[x] &= ~(1 << (y & 0x07));
    }
}

void lcd_set_contrast(uint8_t contrast) {
    lcd_cmd(0x81);
    lcd_cmd(contrast & 0x3F);
}
