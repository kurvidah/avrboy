#!/bin/bash

# Configuration
MCU="atmega328p"
F_CPU="16000000L"
APP_START="0x3000"

if [ -z "$1" ]; then
    echo "Usage: $0 <source_file.c>"
    exit 1
fi

INPUT_FILE=$1
BASENAME=$(basename "$INPUT_FILE" .c)
OUTPUT="${BASENAME^^}.HEX"

echo "Building AVRboy Cartridge: $INPUT_FILE -> $OUTPUT"

# 1. Compile with the correct text section start address
# We use -nostartfiles to keep the app tiny, as the OS handles basics.
# We also include the current directory for avrboy.h
avr-gcc -mmcu=$MCU -DF_CPU=$F_CPU -Os -I. \
    -Wl,-Ttext=$APP_START \
    -nostartfiles \
    "$INPUT_FILE" -o app.elf

# 2. Convert to Intel HEX format
avr-objcopy -O ihex app.elf "$OUTPUT"

# 3. Cleanup
rm app.elf

echo "Done! Copy $OUTPUT to your SD card."
