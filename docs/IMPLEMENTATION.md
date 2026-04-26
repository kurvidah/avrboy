# AVRboy Implementation Progress

This document tracks the technical implementation of the AVRboy deterministic runtime on the ATmega328P (Arduino Nano).

## 1. Core Runtime Environment

The system operates as a **deterministic, cooperative execution platform** running at a fixed 60 FPS.

### 1.1 Execution Model

Implemented in `src/main.c`, the main loop follows the specified model:

1. **Poll I/O**: Captures button and joystick states.
2. **Process Events**: Applications consume events from a centralized ring buffer.
3. **Update State**: Game logic executes.
4. **Render**: Framebuffer is flushed to the ST7567 LCD.
5. **Sync**: System waits for the Timer 1 interrupt (60Hz heartbeat).

### 1.2 System API (ABI)

A stable jump table (`system_api_t`) is implemented in `src/system.h`. This allows external applications to call core functions without knowing their exact flash addresses.

- **Video**: `draw_pixel`, `draw_line`, `fill_rect`, `draw_string`, `lcd_update`, `lcd_clear`.
- **Audio**: `play_tone`.
- **Events**: `poll_event`.
- **Timing**: `get_tick`, `wait_tick`.
- **Debug**: `log` (Serial UART).

## 2. Implemented Subsystems

### 2.1 Event System (`src/event.c`)

- **Type**: Ring buffer with 32 slots (optimized for SRAM).
- **Concurrency**: `event_push` is atomic (interrupt-safe).
- **Events**: Supports `EVENT_BTN_DOWN`, `EVENT_BTN_UP`, `EVENT_JOY_MOVE`, and `EVENT_TICK`.

### 2.2 Timing System (`src/timer.c`)

- **Hardware**: Timer 1 in CTC mode.
- **Resolution**: 60Hz system tick.
- **Clock Support**: Automatically calculates prescalers based on `F_CPU` (supports 8MHz and 16MHz).

### 2.3 Input Subsystem (`src/input.c`)

- **Digital**: 6 buttons mapped to **PORTC (A0-A5)** for physical adjacency.
- **Analog**: 2-axis joystick mapped to **A6/A7** (Analog-only pins on Nano).
- **Processing**: Software debouncing for digital inputs and threshold-based filtering for analog move events.

### 2.4 Audio Subsystem (`src/audio.c`)

- **Hardware**: Timer 2 PWM on **PD3 (Digital Pin 3)**.
- **Logic**: CTC frequency generation with automatic prescaler switching for a wide range of tones.

### 2.5 Video Subsystem (`src/lcd.c`, `src/gfx.c`)

- **Display**: ST7567 128x64 Mono LCD via SPI.
- **Optimization**:
  - 1024-byte static framebuffer.
  - **PROGMEM Font**: 5x7 ASCII font moved to Flash memory to save ~500 bytes of SRAM.
  - Graphics primitives for lines, rects, circles, and triangles.

### 2.6 Debugging System (`src/uart.c`)

- **Interface**: Hardware UART at 115200 baud.
- **Feature**: `system_api.log()` provides formatted `printf` output with a safe 48-byte internal buffer.

## 3. Hardware Configuration (Arduino Nano)

| Component     | Pin       | Note           |
| ------------- | --------- | -------------- |
| **LCD SCK**   | PB5 (D13) | SPI Clock      |
| **LCD MOSI**  | PB3 (D11) | SPI Data       |
| **LCD CS**    | PB2 (D10) | Chip Select    |
| **LCD A0**    | PB0 (D8)  | Data/Command   |
| **LCD RST**   | PB1 (D9)  | Hardware Reset |
| **Button UP** | PC0 (A0)  |                |
| **Button DN** | PC1 (A1)  |                |
| **Button LT** | PC2 (A2)  |                |
| **Button RT** | PC3 (A3)  |                |
| **Button A**  | PC4 (A4)  |                |
| **Button B**  | PC5 (A5)  |                |
| **Joy X**     | ADC6 (A6) |                |
| **Joy Y**     | ADC7 (A7) |                |
| **Audio**     | PD3 (D3)  | PWM Output     |
| **Serial**    | PD0/PD1   | USB Debugging  |

## 4. Memory Footprint (Approximate)

- **Flash Usage**: ~15% (4.7 KB / 30 KB)
- **SRAM Usage**: ~60% (1.2 KB / 2 KB)
  - Framebuffer: 1024 bytes
  - Event Queue: 96 bytes
  - Stack/Globals: ~100 bytes
  - **Remaining for Apps**: ~800 bytes
