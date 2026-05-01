#include "avrboy.h"

static int16_t x, y;
static int8_t dx, dy;

BOOT_ENTRY()

void app_render(void) {
    system_api->draw_string(2, 2, "BOUNCING BALL", 1);
    system_api->fill_rect(x, y, 4, 4, 1);
    system_api->draw_line(0, 11, 127, 11, 1);
    system_api->draw_line(0, 63, 127, 63, 1);
}

int main(void) {
    // Manual initialization for -nostartfiles
    x = 64; y = 32;
    dx = 2; dy = 1;

    system_api->log("APP: Ball Start");
    system_api->set_render_callback(app_render);

    bool running = true;
    while(running) {
        x += dx; y += dy;
        if (x <= 0 || x >= 124) dx = -dx;
        if (y <= 12 || y >= 59) dy = -dy;

        Event e;
        while (system_api->poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if ((e.data1 & (BTN_A | BTN_B)) == (BTN_A | BTN_B)) {
                    running = false;
                }
            }
        }
        system_api->lcd_update();
        system_api->wait_tick();
    }
    return 0;
}
