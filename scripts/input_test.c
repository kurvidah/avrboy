#include "avrboy.h"
#include <avr/interrupt.h>

BOOT_ENTRY()

static uint8_t buttons = 0;
static uint32_t last_log = 0;

void render(void) {
    system_api.draw_string(2, 1, "INPUT TESTER", 1);
    system_api.draw_line(0, 10, 127, 10, 1);
    
    // Display raw button bitmask
    system_api.draw_string(2, 15, "BTNS:", 1);
    for (int i = 0; i < 6; i++) {
        if (buttons & (1 << i)) {
            system_api.fill_rect(40 + (i * 10), 15, 8, 8, 1);
        } else {
            system_api.draw_line(40 + (i * 10), 15, 48 + (i * 10), 23, 1);
        }
    }
    
    system_api.draw_string(2, 30, "A: FLASH APP", 1);
    system_api.draw_string(2, 40, "B: EXIT TO OS", 1);
}

MAIN_ENTRY() {
    sei(); // Enable interrupts for events
    
    system_api.log("Input Test Init");
    system_api.set_render_callback(render);
    
    while (1) {
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                buttons = e.data1;
                
                // Periodic log to avoid spamming UART
                if (system_api.get_tick() - last_log > 100) {
                    system_api.log("BTN DN: %02X", buttons);
                    last_log = system_api.get_tick();
                }

                if (buttons & BTN_B) {
                    ((void (*)(void))0x0000)();
                }
            } else if (e.type == EVENT_BTN_UP) {
                buttons = e.data1;
            }
        }
        
        system_api.lcd_update();
        system_api.wait_tick();
    }
    
    return 0;
}
