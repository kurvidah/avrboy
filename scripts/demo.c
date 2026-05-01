#include "avrboy.h"

static int16_t posX = 64, posY = 32;
static uint16_t frame_count = 0;

void render(void) {
    system_api.draw_string(2, 1, "AVRBOY DEMO", 1);
    system_api.draw_line(0, 10, 127, 10, 1);
    
    // Draw an interactive pixel
    system_api.fill_rect(posX - 2, posY - 2, 5, 5, 1);
    
    char buf[16];
    // We don't have sprintf easily in small apps, but let's see if it works
    // system_api.log("Frame: %d", frame_count);
}

APP_MAIN() {
    system_api.log("Demo started");
    system_api.set_render_callback(render);
    
    while (1) {
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if (e.data1 & BTN_UP)    posY--;
                if (e.data1 & BTN_DOWN)  posY++;
                if (e.data1 & BTN_LEFT)  posX--;
                if (e.data1 & BTN_RIGHT) posX++;
                
                if (e.data1 & BTN_B) {
                    // Soft reset to OS
                    system_api.log("Exiting to OS...");
                    ((void (*)(void))0x0000)();
                }
            }
        }
        
        frame_count++;
        system_api.lcd_update();
        system_api.wait_tick();
    }
    
    return 0;
}
