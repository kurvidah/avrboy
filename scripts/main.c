#include "avrboy.h"

static int16_t x = 64, y = 32;
static int8_t dx = 2, dy = 1;

void app_render(void) {
    system_api.draw_string(2, 2, "BOUNCING BALL", 1);
    system_api.fill_rect(x, y, 4, 4, 1);
    
    // Boundary lines
    system_api.draw_line(0, 11, 127, 11, 1);
    system_api.draw_line(0, 63, 127, 63, 1);
}

int main(void) {
    system_api.set_render_callback(app_render);
    system_api.log("Ball App Loaded");

    while(1) {
        x += dx;
        y += dy;

        if (x <= 0 || x >= 124) dx = -dx;
        if (y <= 12 || y >= 59) dy = -dy;

        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if (e.data1 == (BTN_A | BTN_B)) {
                    // Soft reset to return to OS
                    ((void (*)(void))0x0000)();
                }
            }
        }

        system_api.lcd_update();
        system_api.wait_tick();
    }
}
