#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 128
#define APP_START 0x2000

// ---- External function (you already have this) ----
extern void flash_page(uint16_t addr, uint8_t* data);

// ---- Utilities ----
static uint8_t hex_to_byte(char high, char low) {
    uint8_t h = (high <= '9') ? (high - '0') : (high - 'A' + 10);
    uint8_t l = (low  <= '9') ? (low  - '0') : (low  - 'A' + 10);
    return (h << 4) | l;
}

// ---- Parser State ----
static uint32_t ext_addr = 0;  // upper 16 bits
static uint8_t page_buf[PAGE_SIZE];
static uint16_t current_page = 0xFFFF;
static bool page_dirty = false;

// ---- Flush Page ----
static void flush_page(void) {
    if (page_dirty) {
        flash_page(current_page, page_buf);
        page_dirty = false;
    }
}

// ---- Initialize ----
void hex_parser_init(void) {
    ext_addr = 0;
    current_page = 0xFFFF;
    page_dirty = false;
}

// ---- Process One Line ----
// line must be null-terminated string ":..."
bool hex_parse_line(const char* line) {
    if (line[0] != ':') return false;

    uint8_t len  = hex_to_byte(line[1], line[2]);
    uint16_t addr = (hex_to_byte(line[3], line[4]) << 8) |
                     hex_to_byte(line[5], line[6]);
    uint8_t type = hex_to_byte(line[7], line[8]);

    const char* data_ptr = &line[9];

    // ---- Handle record types ----
    if (type == 0x00) {
        // DATA RECORD
        for (uint8_t i = 0; i < len; i++) {
            uint8_t byte = hex_to_byte(data_ptr[i*2], data_ptr[i*2 + 1]);

            uint32_t full_addr = (ext_addr << 16) | addr;
            uint16_t final_addr = (uint16_t)(full_addr & 0xFFFF);

            // Skip anything below application region
            if (final_addr < APP_START) {
                addr++;
                continue;
            }

            uint16_t page_base = final_addr & ~(PAGE_SIZE - 1);

            // If new page → flush old
            if (page_base != current_page) {
                flush_page();

                current_page = page_base;

                // Initialize page buffer (important!)
                for (uint8_t j = 0; j < PAGE_SIZE; j++) {
                    page_buf[j] = 0xFF;
                }
            }

            uint16_t offset = final_addr & (PAGE_SIZE - 1);
            page_buf[offset] = byte;

            page_dirty = true;

            addr++;
        }
    }
    else if (type == 0x01) {
        // EOF
        flush_page();
        return false; // signal done
    }
    else if (type == 0x04) {
        // Extended Linear Address
        ext_addr = (hex_to_byte(data_ptr[0], data_ptr[1]) << 8) |
                   hex_to_byte(data_ptr[2], data_ptr[3]);
    }

    return true;
}