#include "bl_if.h"
#include <stdint.h>
#include <string.h>

extern void bootloader_main(void);

void flash_page(uint16_t addr, uint8_t* data) {
    volatile bl_cmd_t* cmd = BL_CMD_ADDR;

    cmd->cmd = BL_CMD_WRITE;
    cmd->addr = addr;
    memcpy((void*)cmd->data, data, 128);
    cmd->status = BL_STATUS_IDLE;

    // Call the bootloader function in BLS
    bootloader_main();
}

void jump_to_app(void) {
    void (*app_reset)(void) = (void (*)(void))0x3000;
    app_reset();
}
