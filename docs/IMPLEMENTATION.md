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

| Component     | Pin       | Note                |
| ------------- | --------- | ------------------- |
| **LCD CS**    | PB2 (D10) | SPI Chip Select     |
| **SD CS**     | PD4 (D4)  | SPI Chip Select     |
| **Buttons**   | PC0-PC5   | UP, DN, LT, RT, A, B|
| **Joystick**  | ADC6/ADC7 | Analog X, Y         |
| **Audio**     | PD3 (D3)  | PWM / Square Wave   |

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
