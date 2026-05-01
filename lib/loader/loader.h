#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes the Intel HEX parser state.
 */
void hex_parser_init(void);

/**
 * @brief Parses a single line of Intel HEX.
 * 
 * This function will buffer data and flash it to the application region
 * when a full page is accumulated or the end of file is reached.
 * 
 * @param line The HEX line string.
 * @return true if parsing/flashing continues, false on error or EOF.
 */
bool hex_parse_line(const char* line);

/**
 * @brief Jumps to the application entry point.
 * 
 * This function should perform necessary cleanup before jumping.
 */
void jump_to_app(void) __attribute__((noreturn));

/**
 * @brief Bootloader entry point (linked at 0x7000).
 * 
 * This is used if the system is started in bootloader mode.
 */
void bootloader_main(void) __attribute__((section(".bootloader")));

#endif // LOADER_H
