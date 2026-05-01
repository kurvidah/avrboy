#include "loader.h"
#include "system.h"
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// The application start address as defined in docs/IMPLEMENTATION.md
#define APP_START_ADDRESS 0x3000

// Flash page size for ATmega328P is 128 bytes
#define PAGE_SIZE 128

void bootloader_flash_page(uint32_t page_addr, uint8_t *buf) __attribute__((section(".bootloader")));

void bootloader_flash_page(uint32_t page_addr, uint8_t *buf) {
    uint16_t i;
    uint8_t sreg;

    // Disable interrupts
    sreg = SREG;
    cli();

    boot_page_erase(page_addr);
    boot_spm_busy_wait();

    for (i = 0; i < PAGE_SIZE; i += 2) {
        // Set up word from buffer
        uint16_t w = buf[i] | (buf[i + 1] << 8);
        boot_page_fill(page_addr + i, w);
    }

    boot_page_write(page_addr);
    boot_spm_busy_wait();

    boot_rww_enable();

    // Re-enable interrupts
    SREG = sreg;
}

void jump_to_app(void) {
    system_api.log("JUMP: 0x%04X", APP_START_ADDRESS);
    _delay_ms(100); // Give UART time to flush
    
    // Disable interrupts
    cli();

    // According to docs, we should reset peripherals or just jump.
    // The OS handles most of it.
    
    // Jump to the application start address.
    // AVR uses word addresses for jumping in some contexts, 
    // but GCC's function pointer jump uses byte addresses.
    void (*app_entry)(void) = (void (*)(void))(APP_START_ADDRESS);
    app_entry();

    // Should never reach here
    while (1);
}

void bootloader_main(void) {
    // Basic bootloader entry point if needed
    // For now, it just jumps to the OS or waits.
    system_api.log("BOOT: Entry");
    _delay_ms(100);
    
    // If we are here, maybe we should just go to the OS (0x0000)
    // But usually the OS starts at 0x0000 and we are called from there.
}
