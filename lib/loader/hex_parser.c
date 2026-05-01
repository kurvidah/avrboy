#include "loader.h"
#include "system.h"
#include "uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern void bootloader_flash_page(uint32_t page_addr, uint8_t *buf);

#define PAGE_SIZE 128
#define APP_START_ADDRESS 0x4000

static uint8_t page_buffer[PAGE_SIZE];
static uint32_t current_page_addr = 0xFFFFFFFF;
static bool page_dirty = false;

static uint8_t hex_to_byte(const char* hex) {
    char tmp[3];
    tmp[0] = hex[0];
    tmp[1] = hex[1];
    tmp[2] = 0;
    return (uint8_t)strtol(tmp, NULL, 16);
}

static uint16_t hex_to_word(const char* hex) {
    char tmp[5];
    memcpy(tmp, hex, 4);
    tmp[4] = 0;
    return (uint16_t)strtol(tmp, NULL, 16);
}

void hex_parser_init(void) {
    memset(page_buffer, 0xFF, PAGE_SIZE);
    current_page_addr = 0xFFFFFFFF;
    page_dirty = false;
}

static void flush_page(void) {
    if (page_dirty && current_page_addr != 0xFFFFFFFF) {
        uart_log("FLSH: 0x%04lX", current_page_addr);
        bootloader_flash_page(current_page_addr, page_buffer);
    }
    memset(page_buffer, 0xFF, PAGE_SIZE);
    page_dirty = false;
}

bool hex_parse_line(const char* line) {
    if (line[0] != ':') return false;

    uint8_t len = hex_to_byte(line + 1);
    uint16_t addr = hex_to_word(line + 3);
    uint8_t type = hex_to_byte(line + 7);

    // Relocate address to APP_START_ADDRESS only if it's 0-based
    uint32_t target_addr = (uint32_t)addr;
    if (target_addr < APP_START_ADDRESS) {
        target_addr += APP_START_ADDRESS;
    }

    if (type == 0x01) { // EOF
        uart_log("HEX: EOF");
        flush_page();
        return false; 
    }

    if (type == 0x00) { // Data
        for (uint8_t i = 0; i < len; i++) {
            uint8_t byte = hex_to_byte(line + 9 + (i * 2));
            uint32_t byte_addr = target_addr + i;
            uint32_t page_addr = byte_addr & ~(PAGE_SIZE - 1);

            if (page_addr != current_page_addr) {
                flush_page();
                current_page_addr = page_addr;
            }

            page_buffer[byte_addr % PAGE_SIZE] = byte;
            page_dirty = true;
        }
    }

    return true;
}
