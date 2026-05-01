#!/bin/bash

# Configuration
MCU="atmega328p"
F_CPU="16000000L"
APP_START="0x4000"

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
# We move .data to 0x800540 to avoid overwriting the OS data starting at 0x0140.
# We explicitly place the .entry section at 0x4000.
avr-gcc -mmcu=$MCU -DF_CPU=$F_CPU -Os -I. \
    -Wl,--section-start=.entry=$APP_START \
    -Wl,-Ttext=0x4004 \
    -Wl,--section-start=.data=0x800540 \
    -nostartfiles \
    "$INPUT_FILE" -o app.elf

# 2. Convert to Intel HEX format
avr-objcopy -O ihex app.elf "$OUTPUT"

# 3. Cleanup
rm app.elf

echo "Done! Copy $OUTPUT to your SD card."
