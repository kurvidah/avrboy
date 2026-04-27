# AVRboy Implementation Progress

## 1. Core Architecture

### 1.1 Transparent Paging (Video Optimization)
The system uses a **128-byte page buffer**. 
- **Mechanism**: The screen is divided into 8 pages. 
- **Loop**: `system_api.lcd_update()` executes the render callback 8 times.
- **SRAM**: ~1.4KB free for apps.

### 1.2 Application Loader
- **Location**: Applications are flashed to **0x3000** (12KB offset).
- **Format**: Intel HEX from SD card.
- **Auto-Offset**: The OS automatically relocates 0-based HEX files to 0x3000.
- **Constraint**: While the loader offsets the code, applications **must be compiled with a base address of 0x3000** (e.g., `-Wl,--section-start=.text=0x3000`) for absolute jumps and calls to work correctly.

---

## 2. Updated Hardware Configuration

| Component     | Pin       | Note                |
| ------------- | --------- | ------------------- |
| **LCD CS**    | PB2 (D10) | SPI Chip Select     |
| **SD CS**     | PD4 (D4)  | SPI Chip Select     |
| **Buttons**   | PC0-PC5   | UP, DN, LT, RT, A, B|
| **Joystick**  | ADC6/ADC7 | Analog X, Y         |
| **Audio**     | PD3 (D3)  | PWM Output          |

---

## 3. Flashing Flow
1. **Select**: Use UP/DN to choose a `.HEX` file.
2. **Trigger**: Press **A** or **RIGHT**.
3. **Wait**: The OS waits for you to **release the button** before flashing.
4. **Flash**: Loader writes to Flash memory at 0x3000.
5. **Jump**: OS resets peripherals and jumps to 0x3000.
