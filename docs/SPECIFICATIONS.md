# 1. System Overview

## 1.1 Purpose

Design and implement a **deterministic, event-driven embedded execution platform** capable of:

* Loading applications (“cartridges”) via a dedicated bootloader interface
* Providing a stable API for application development (ABI)
* Handling real-time input (Digital, Analog, IMU), audio, and video output
* Operating within strict 8-bit microcontroller constraints (ATmega328P)

---

## 1.2 System Definition

> A **single-core, non-preemptive embedded runtime** with interrupt-driven input, fixed-step execution (60 FPS), and a statically defined ABI via a global jump table.

---

## 1.3 Design Goals

* Deterministic execution (bounded latency)
* Minimal runtime overhead
* Stable API for third-party applications
* Hardware–software co-design simplicity
* Fully offline operation

---

# 2. Functional Requirements

## 2.1 Application Execution

* The system **shall load an application** into flash memory starting at `0x2000`
* The system **shall transfer control to the application region** after successful loading
* The application **shall execute in a cooperative loop model** synchronized to the system tick

---

## 2.2 Event System

* The system **shall capture input via interrupts and polling**
* The system **shall enqueue events into a global event buffer**
* The system **shall expose events to applications via the `poll_event` API**
* The event queue **shall be a non-blocking ring buffer**

---

## 2.3 Input Handling

* The system **shall support digital inputs** (6 buttons: UP, DOWN, LEFT, RIGHT, A, B)
* The system **shall support analog inputs** (ADC-based joystick for X/Y axes)
* The system **shall support inertial motion sensing** via I2C (MPU6050)

---

## 2.4 Video Output

* The system **shall drive an external 128x64 monochrome LCD via SPI**
* The system **shall support a full 1024-byte framebuffer**
* The system **shall provide rendering primitives** (pixel, line, rect, string) via API

---

## 2.5 Audio Output

* The system **shall generate audio via Timer 2 in CTC mode**
* The system **shall support square-wave tone generation**

---

## 2.6 Storage / Cartridge System

* The system **shall accept application data in Intel HEX format**
* The system **shall implement a bootloader interface** to write pages (128 bytes) to flash
* *Planned:* The system will eventually read application binaries from microSD via SPI/FATfs.

---

## 2.7 Timing System

* The system **shall implement a system tick (60 Hz)** via Timer 1
* The main loop **shall synchronize to this tick** via `wait_tick()`

---

# 3. Non-Functional Requirements

## 3.1 Performance

* Maximum interrupt latency: **< 50 µs**
* Frame duration: **16.67 ms (60 FPS)**
* Event processing must complete within one frame

---

## 3.2 Memory Constraints

ATmega328P limits:

* Flash: 32 KB (Application region starts at 8KB / `0x2000`)
* SRAM: 2 KB
* EEPROM: 1 KB

### Allocation Targets

| Component   | Budget     |
| ----------- | ---------- |
| Framebuffer | 1024 B     |
| Event Queue | 64 B       |
| Stack       | 256–512 B  |
| Game State  | Remaining  |

---

## 3.3 Reliability

* No dynamic memory allocation (malloc-free design)
* All buffers must be statically bounded
* System must avoid undefined behavior under overflow conditions

---

## 3.4 Determinism

* All critical operations must have predictable execution time
* Interrupt handlers must be minimal and bounded

---

# 4. System Architecture

## 4.1 Layered Model

```
[ Hardware: ATmega328P + LCD + MPU6050 ]
     ↓
[ Drivers: SPI, TWI, ADC, Timer, UART ]
     ↓
[ Runtime: Event Queue + System API Table ]
     ↓
[ Application: User Cartridge ]
```

---

## 4.2 Execution Model

```c
while (1) {
    input_poll();          // Poll hardware to generate events
    update_game_state();   // Application logic (polls event queue)
    render_frame();        // Application rendering
    system_api.wait_tick(); // Synchronize to 60 FPS
}
```

---

# 5. API Specification (ABI)

## 5.1 System API Table (`system_api_t`)

A **fixed jump table** provided by the runtime:

```c
typedef struct {
    // Video
    void (*draw_pixel)(uint8_t x, uint8_t y, uint8_t color);
    void (*draw_line)(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
    void (*fill_rect)(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
    void (*draw_string)(uint8_t x, uint8_t y, const char *s, uint8_t color);
    void (*lcd_update)(void);
    void (*lcd_clear)(void);

    // Audio
    void (*play_tone)(uint16_t freq);

    // Input / Events
    bool (*poll_event)(Event* e);

    // Timing
    uint32_t (*get_tick)(void);
    void (*wait_tick)(void);

    // MPU6050
    void (*mpu_read)(void* data);

    // Debugging
    void (*log)(const char *fmt, ...);
} system_api_t;
```

---

# 6. Event System

## 6.1 Event Structure

```c
typedef struct {
    uint8_t type;   // event_type_t
    uint8_t data1;  // Bitmask for buttons or X-axis for joystick
    uint8_t data2;  // Y-axis for joystick
} Event;
```

---

## 6.2 Event Types

* `EVENT_NONE` (0)
* `EVENT_BTN_DOWN` (1) - `data1` contains button bitmask
* `EVENT_BTN_UP` (2) - `data1` contains button bitmask
* `EVENT_JOY_MOVE` (3) - `data1` = X, `data2` = Y
* `EVENT_TICK` (4)
* `EVENT_CART_INSERT` (5)

---

## 6.3 Input Bitmasks

```c
#define BTN_UP    (1 << 0)
#define BTN_DOWN  (1 << 1)
#define BTN_LEFT  (1 << 2)
#define BTN_RIGHT (1 << 3)
#define BTN_A     (1 << 4)
#define BTN_B     (1 << 5)
```

---

# 7. Cartridge System

## 7.1 Loading Mechanism

* Applications are currently provided as **Intel HEX files**.
* The **HEX Parser** (`hex_parser.c`) processes records and uses the **Bootloader Interface** (`bl_if.h`) to flash the application into memory.
* Application region: `0x2000` to `0x6FFF`.

---

# 8. Hardware Subsystems

## 8.1 Video (LCD)

* **Display**: 128x64 Monochrome LCD.
* **Controller**: Driven via SPI.
* **Framebuffer**: Row-major, 8 pixels per byte (MSB-first).

## 8.2 Audio

* **Output**: Piezo buzzer / speaker on PD3 (OC2B).
* **Mode**: Timer 2 CTC (Clear Timer on Compare) for precise frequency control.

## 8.3 IMU (MPU6050)

* **Interface**: I2C (TWI) @ 400kHz.
* **Data**: 3-axis Accelerometer, 3-axis Gyroscope, Temperature.

---

# 9. Validation & Testing

## 9.1 Unit Testing

* Event queue correctness (ring buffer wraparound)
* HEX parser record validation

## 9.2 Integration Testing

* Input → Event → Application response
* Stable 60 FPS output under load
* Audio frequency accuracy

---

# 14. Risks and Limitations

* **RAM Overflow**: Framebuffer (1KB) uses half of the available SRAM.
* **Flash Wear**: Frequent cartridge reloading affects flash longevity.
* **I2C/SPI Conflicts**: Shared bus timing during high-speed rendering.

---

# Final Assessment

The current implementation provides a **stable 8-bit execution environment** with a unified API for graphics, audio, and motion sensing, successfully abstracting the hardware for application developers.
