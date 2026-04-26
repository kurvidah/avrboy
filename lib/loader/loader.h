#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdbool.h>

// Hex Parser
void hex_parser_init(void);
bool hex_parse_line(const char* line);

// Loader
void flash_page(uint16_t addr, uint8_t* data);
void jump_to_app(void);

#endif // LOADER_H
