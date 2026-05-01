# 1. System Overview

## 1.1 Purpose
A deterministic, event-driven embedded execution platform for the ATmega328P, supporting SD-based application loading and motion-controlled inputs.

---

# 2. Functional Requirements

## 2.1 Application Loading (Cartridges)
* The system **shall mount a FAT16/FAT32 microSD card** at startup.
* The system **shall list .HEX files** in a scrollable menu.
* The system **shall flash selected programs** into the application region (0x4000-0x6EFF).

## 2.2 Rendering Model (Transparent Paging)
* To conserve RAM, the system **shall use a 128-byte page buffer**.
* The runtime **shall execute the application's render callback 8 times per frame**.
* The API **shall remain pixel-based**, with automatic culling for non-active pages.

## 2.3 Input (Interrupt-Driven)
* The system **shall poll hardware buttons at 60Hz** in the background.
* The system **shall provide an event queue** for applications to process inputs asynchronously.

---

# 3. Memory Map (ATmega328P)

| Region | Address | Size | Description |
| --- | --- | --- | --- |
| **OS Runtime** | `0x0000-0x3FFF` | 16 KB | Resident OS, Drivers, and FATFS |
| **App Space** | `0x4000-0x6EFF` | 12 KB | Flashable Application Area |
| **API Bridge** | `0x6F00` | - | Flash location of `system_api_t` table |
| **Bootloader** | `0x7000-0x7FFF` | 4 KB | Flash Writing Routines (SPM) |

---

# 4. System API (ABI)

The `system_api` table is the primary interface for applications. It is accessed via a RAM bridge at **0x0100**.

### 4.1 Graphics
* `draw_string()`: Renders text using the system 5x7 font.
* `lcd_update()`: Triggers the 8-page rendering loop.
* `set_render_callback()`: Registers the application's drawing function.

### 4.2 Control
* `poll_event()`: Retrieves button/joystick/timer events.
* `play_tone()`: Generates square-wave audio frequencies.
* `wait_tick()`: Synchronizes the application to the 60Hz system heartbeat.

### 4.3 Hardware
* `mpu_read()`: Provides raw 6-axis motion data.
