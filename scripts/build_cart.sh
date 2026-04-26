#!/bin/bash

# Configuration
MCU="atmega328p"
F_CPU="16000000L"
APP_START="0x3000"
OUTPUT="ball_game.hex"

echo "Building AVRboy Cartridge..."

# 1. Compile with the correct text section start address
# We also need to point to the include folder if we were using OS headers
avr-gcc -mmcu=$MCU -DF_CPU=$F_CPU -Os -Wl,-Ttext=$APP_START main.c -o app.elf

# 2. Convert to Intel HEX format
avr-objcopy -O ihex app.elf $OUTPUT

# 3. Cleanup
rm app.elf

echo "Done! Copy $OUTPUT to your SD card."
