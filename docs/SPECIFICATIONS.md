# 1. System Overview

## 1.1 Purpose
A deterministic, event-driven embedded execution platform for the ATmega328P, supporting SD-based application loading and motion-controlled inputs.

---

# 2. Functional Requirements

## 2.1 Application Loading (Cartridges)
* The system **shall mount a FAT16/FAT32 microSD card** at startup.
* The system **shall list .HEX files** in a scrollable menu.
* The system **shall flash selected programs** into the application region (0x2000-0x6FFF).

## 2.2 Rendering Model (Transparent Paging)
* To conserve RAM, the system **shall use a 128-byte page buffer**.
* The runtime **shall execute the application's render callback 8 times per frame**.
* The API **shall remain pixel-based**, with automatic culling for non-active pages.

---

# 3. Memory Map (ATmega328P)

| Region | Address | Size | Description |
| --- | --- | --- | --- |
| **OS Runtime** | 0x0000 | 8 KB | Resident OS & System API |
| **App Space** | 0x2000 | 20 KB | Flashable Application Area |
| **Bootloader** | 0x7000 | 4 KB | Flash Writing Routines (SPM) |

---

# 4. System API (ABI)

The `system_api` table remains the primary interface for applications:
* `lcd_update()`: Triggers the 8-page rendering loop.
* `set_render_callback()`: Registers the application's drawing function.
* `poll_event()`: Retrieves button/joystick/IMU events.
