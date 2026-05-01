#include "loader.h"
#include "system.h"
#include "uart.h"
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define APP_START_ADDRESS 0x4000
#define PAGE_SIZE 128

void bootloader_flash_page(uint32_t page_addr, uint8_t *buf) __attribute__((section(".bootloader")));

void bootloader_flash_page(uint32_t page_addr, uint8_t *buf) {
    uint8_t sreg = SREG;
    cli();

    boot_page_erase(page_addr);
    boot_spm_busy_wait();

    for (uint16_t i = 0; i < PAGE_SIZE; i += 2) {
        uint16_t w = buf[i] | (buf[i + 1] << 8);
        boot_page_fill(page_addr + i, w);
    }

    boot_page_write(page_addr);
    boot_spm_busy_wait();
    boot_rww_enable();

    SREG = sreg;
}

void dump_app_region(void) __attribute__((section(".bootloader")));
void dump_app_region(void) {
    uart_log("DUMP @ 0x4000:");
    for (uint16_t i = 0; i < 16; i++) {
        uint8_t b = pgm_read_byte(APP_START_ADDRESS + i);
        uart_log(" %02X", b);
    }
}

void jump_to_app(void) __attribute__((section(".bootloader"), noreturn));
void jump_to_app(void) {
    dump_app_region();
    uart_log("OS: Jump @ 0x4000");
    _delay_ms(100);
    cli();

    // Word address = Byte address / 2
    uint16_t word_addr = APP_START_ADDRESS >> 1;

    asm volatile (
        "clr r1 \n\t"
        "ijmp   \n\t"
        : 
        : "z" (word_addr)
    );
    while (1);
}

void bootloader_main(void) __attribute__((section(".bootloader")));
void bootloader_main(void) {
    uart_log("BOOT: Entry");
    ((void (*)(void))0x0000)();
}
