// bl_if.h
// Bootloader interface definitions
#pragma once

#include <stdint.h>

#define BL_CMD_ADDR ((volatile bl_cmd_t*)0x0100)

#define BL_CMD_NONE   0
#define BL_CMD_WRITE  1

#define BL_STATUS_IDLE   0
#define BL_STATUS_BUSY   1
#define BL_STATUS_DONE   2
#define BL_STATUS_ERROR  3

typedef struct {
    uint8_t  cmd;
    uint16_t addr;      // flash address (byte address)
    uint8_t  data[128]; // one page
    uint8_t  status;
} bl_cmd_t;

