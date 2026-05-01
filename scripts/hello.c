#include "avrboy.h"

BOOT_ENTRY()

void app_render(void) {
    system_api->draw_string(2, 2, "Hello, World!", 1);
    system_api->draw_string(2, 50, "PRESS B TO EXIT", 1);
}

int main(void) {
    // Crucial: Initialize variables inside main due to -nostartfiles
    system_api->log("APP: Hello Start");
    system_api->set_render_callback(app_render);

    bool running = true;
    while(running) {
        Event e;
        while (system_api->poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN && (e.data1 & BTN_B)) {
                running = false;
            }
        }

        system_api->lcd_update();
        system_api->wait_tick();
    }

    return 0;
}
