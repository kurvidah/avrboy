#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <source_file.c>"
    exit 1
fi

INPUT_FILE=$1
OUTPUT="avrboy_sim"

echo "Building AVRboy Simulator with: $INPUT_FILE"

gcc -DSIMULATOR -I../scripts \
    main.c "../scripts/$INPUT_FILE" \
    -o "$OUTPUT" \
    $(pkg-config --cflags --libs sdl2)

echo "Done! Run ./$OUTPUT to start simulation."
