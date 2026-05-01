# AVRboy - Embedded Game Platform

AVRboy is a deterministic, event-driven embedded execution platform for the ATmega328P. It features a transparent paging video system, an interrupt-driven input engine, and a desktop simulator for rapid prototyping.

---

## Quick Start (Simulation)

To develop and test applications without hardware, use the SDL2-based simulator.

**Prerequisites:** `libsdl2-dev`

```bash
# Build an application for the simulator
cd simulator
./build_sim.sh ../scripts/joy_visualizer.c

# Run the simulation
./avrboy_sim
```

- **Controls:** Arrow Keys (D-Pad), Z/X (A/B Buttons), WASD (Analog Joystick).

---

## Hardware Developer Workflow

### 1. Flash the OS

The OS handles SD card mounting, application flashing (Intel HEX), and the System API.

```bash
# Using PlatformIO
pio run --target upload
```

### 2. Build a Cartridge

Applications are compiled as standalone `.HEX` files that start at address `0x4000`.

```bash
cd scripts
./build_cart.sh my_game.c
```

- Copy the resulting `MY_GAME.HEX` to a FAT16/FAT32 formatted SD card.
- Insert into AVRboy, select from the menu, and press **A** to flash.

---

## System Architecture

### Memory Map (ATmega328P)

| Region         | Address         | Size  | Description                            |
| -------------- | --------------- | ----- | -------------------------------------- |
| **OS Runtime** | `0x0000-0x3FFF` | 16 KB | Resident OS, Drivers, and FATFS        |
| **App Space**  | `0x4000-0x6EFF` | 12 KB | Flashable Application Area             |
| **API Bridge** | `0x6F00`        | -     | Flash location of `system_api_t` table |
| **Bootloader** | `0x7000-0x7FFF` | 4 KB  | SPM Flash writing routines             |

### RAM Layout

- **`0x0100`**: System API Bridge (The OS copies the API table here at boot).
- **`0x0140`**: OS Data / Stack.
- **`0x0540`**: Application Data (Cartridges are linked with `.data=0x800540`).

---

## Application API

Applications must include `avrboy.h` and use the `APP_MAIN()` macro. This macro automatically handles interrupt enablement and cross-platform entry points.

### Core API Functions (`system_api`)

- `draw_string(x, y, str, color)`: Render text using the 5x7 system font.
- `lcd_update()`: Trigger the 8-page rendering loop (Transparent Paging).
- `poll_event(&event)`: Retrieve the next hardware event (Buttons, Joystick, Tick).
- `play_tone(freq)`: Generate a square-wave tone (0 to stop).
- `wait_tick()`: Sync to the system 60Hz heartbeat.

### Example Application

```c
#include "avrboy.h"

void my_render() {
    system_api.draw_string(10, 10, "HELLO!", 1);
}

APP_MAIN() {
    system_api.set_render_callback(my_render);
    while(1) {
        system_api.lcd_update();
        system_api.wait_tick();
    }
}
```

---

## Included Demos

- `input.c`: Real-time visual and audio feedback for all inputs.
- `ball.c`: Non-interactive animation to verify display stability.
