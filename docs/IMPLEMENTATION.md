# AVRboy Implementation Progress

## 1. Core Architecture

### 1.1 Transparent Paging (Video Optimization)

The system uses a **128-byte page buffer**.

- **Mechanism**: The screen is divided into 8 horizontal pages (128x8 pixels each).
- **Loop**: `system_api.lcd_update()` executes the application's render callback 8 times per frame.
- **SRAM**: ~1KB reserved for OS/Bridge, ~1KB available for apps.

### 1.2 Application Loader

- **Location**: Applications are flashed to **0x4000** (16KB offset).
- **Format**: Intel HEX from SD card.
- **Auto-Offset**: The OS automatically relocates 0-based HEX files to 0x4000.
- **Initialization**: Applications use the standard C Runtime (CRT) to initialize RAM variables before `main()` is called.

### 1.3 System API Bridge (Harvard Architecture)

- **Problem**: AVR function pointers cannot directly reference tables in Flash (`PROGMEM`).
- **Solution**: At boot, the OS copies the `system_api_t` table from Flash (`0x6F00`) to a fixed RAM address (**0x0100**).
- **Usage**: Applications access the OS via `avrboy.h`, which points to this stable RAM bridge.

---

## 2. Hardware Configuration

| Component        | Pin        | Port  | Note                |
| ---------------- | ---------- | ----- | ------------------- |
| **LCD CS**       | PB2 (D10)  | PORTB | SPI Chip Select     |
| **LCD AO**       | PB0 (D8)   | PORTB | Data/Command Select |
| **LCD SDA**      | PB3 (MOSI) | PORTB | Serial Data         |
| **LCD SCK**      | PB5 (SCK)  | PORTB | Serial Clock        |
| **LCD RESET**    | PB1 (D9)   | PORTB | Hardware Reset      |
| **SD CS**        | PD4 (D4)   | PORTD | SPI Chip Select     |
| **SD MOSI**      | PB3 (MOSI) | PORTB | Serial Data         |
| **SD MISO**      | PB4 (MISO) | PORTB | Serial Data         |
| **SD SCK**       | PB5 (SCK)  | PORTB | Serial Clock        |
| **Audio**        | PD3 (D3)   | PORTD | PWM / Square Wave   |
| **MPU6050 SDA**  | PC4 (A4)   | PORTC | Hardware I2C        |
| **MPU6050 SCL**  | PC5 (A5)   | PORTC | Hardware I2C        |
| **Joystick X**   | PC0 (A0)   | PORTC | ADC Channel 0       |
| **Joystick Y**   | PC1 (A1)   | PORTC | ADC Channel 1       |
| **Button UP**    | PC2 (A2)   | PORTC | Digital Input       |
| **Button DOWN**  | PC3 (A3)   | PORTC | Digital Input       |
| **Button LEFT**  | PD2 (D2)   | PORTD | Digital Input       |
| **Button RIGHT** | PD5 (D5)   | PORTD | Digital Input       |
| **Button A**     | PD6 (D6)   | PORTD | Digital Input       |
| **Button B**     | PD7 (D7)   | PORTD | Digital Input       |

---

## 3. Input Engine

- **Interrupt Driven**: Button states are sampled at **60Hz** inside the System Timer ISR.
- **Throttled ADC**: Analog joystick axes are sampled at **~15Hz** to reduce CPU overhead while maintaining smooth movement.
- **Event Queue**: A 32-slot circular buffer ensures no button presses are missed during heavy processing.

---

## 4. Flashing Flow

1. **Select**: Use UP/DN to choose a `.HEX` file from the SD root.
2. **Trigger**: Press **A** or **RIGHT**.
3. **Wait**: The OS waits for you to **release the button** before flashing.
4. **Flash**: Loader writes to Flash memory at 0x4000.
5. **Jump**: OS performs a word-address `ijmp` to 0x2000 (0x4000 byte).

---

## 5. Technical Challenges & Lessons Learned

Developing the AVRboy bootloader and application bridge revealed several critical hardware and compiler-level constraints unique to the ATmega328P.

### 5.1 The Harvard Architecture Trap

On the AVR architecture, **Flash (Code)** and **RAM (Data)** occupy separate address spaces. Standard C pointers can only read from RAM.

- **The Challenge**: The `system_api_t` function table was stored in Flash (`PROGMEM`). When an application tried to call a function via `system_api->log()`, the CPU attempted to read the pointer from RAM, leading to immediate crashes.
- **The Solution**: A physical **RAM Bridge** was established at `0x0100`. The OS physically copies the API table from Flash to RAM during boot, allowing applications to use standard pointer dereferencing safely.

### 5.2 Byte vs. Word Addressing

The ATmega328P uses byte addressing for data but **word addressing** for its instruction pointer and jump instructions.

- **The Challenge**: Telling the CPU to jump to the byte address `0x4000` via the `ijmp` instruction resulted in the CPU jumping to word address `0x4000` (which is byte address `0x8000`). This caused the system to jump outside of physical memory and reset.
- **The Solution**: The jump target must be bit-shifted (`target >> 1`) before being loaded into the `Z` register for the `ijmp` operation.

### 5.3 Linker Optimization (LTO)

Modern compilers use Link-Time Optimization to strip "unused" code.

- **The Challenge**: Since the OS only references the bootloader during the jump sequence, the compiler frequently optimized away or merged the `.bootloader` section, ignoring the strict `0x7000` requirement.
- **The Solution**: LTO was explicitly disabled (`-fno-lto`) and specific linker flags (`--undefined`) were used to force the retention of critical jumping and flashing symbols.

### 5.4 The Missing Runtime (Standard CRT)

Initially, applications were compiled with `-nostartfiles` to minimize size.

- **The Challenge**: This removed the "Startup Code" responsible for copying `.data` from Flash to RAM and zeroing `.bss`. Consequently, any global variables or string literals in applications were garbage or uninitialized, causing hangs.
- **The Solution**: Switched to the standard C Runtime for all applications. This ensures that every cartridge is a valid, self-initializing program before its `main()` executes.

### 5.5 Memory Overlap & OS Growth

As the OS added support for SD, FATFS, and the MPU6050, its size grew beyond the initial 8KB limit.

- **The Challenge**: Applications starting at `0x3000` (12KB) were being overwritten by the OS, or were overwriting the OS during the flashing process.
- **The Solution**: The application area was pushed back to **`0x4000` (16KB)**, providing a massive "safe zone" for OS growth and ensuring a clean physical separation between system and user code.
