// bootloader.c
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "bl_if.h"

// ---- Configuration ----
#define APP_START        0x2000
#define BOOT_START       0x7000
#define PAGE_SIZE        128

// ---- Entry point ----
void bootloader_main(void) __attribute__((naked, section(".bootloader")));

void bootloader_main(void) {
    // No prologue/epilogue (naked)
    asm volatile ("cli");

    volatile bl_cmd_t* cmd = BL_CMD_ADDR;

    if (cmd->cmd != BL_CMD_WRITE) {
        cmd->status = BL_STATUS_ERROR;
        asm volatile ("sei");
        return;
    }

    cmd->status = BL_STATUS_BUSY;

    uint16_t addr = cmd->addr;

    // ---- Bounds check ----
    if (addr < APP_START || addr >= BOOT_START) {
        cmd->status = BL_STATUS_ERROR;
        asm volatile ("sei");
        return;
    }

    // Align to page
    addr &= ~(PAGE_SIZE - 1);

    // ---- Erase page ----
    boot_page_erase(addr);
    boot_spm_busy_wait();

    // ---- Fill page buffer ----
    for (uint16_t i = 0; i < PAGE_SIZE; i += 2) {
        uint16_t w = cmd->data[i] | (cmd->data[i+1] << 8);
        boot_page_fill(addr + i, w);
    }

    // ---- Write page ----
    boot_page_write(addr);
    boot_spm_busy_wait();

    // Re-enable RWW section
    boot_rww_enable();

    cmd->status = BL_STATUS_DONE;
    cmd->cmd = BL_CMD_NONE;

    asm volatile ("sei");
    asm volatile ("ret");
    }