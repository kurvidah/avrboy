#include "avrboy.h"
#include <avr/interrupt.h>

static int16_t x = 64, y = 32;
static int8_t dx = 2, dy = 1;

void render(void) {
    // Draw background/header
    system_api.draw_string(2, 1, "BOUNCING BALL", 1);
    system_api.draw_line(0, 10, 127, 10, 1);
    
    // Draw the ball
    system_api.fill_rect(x, y, 4, 4, 1);
}

APP_MAIN() {
    system_api.log("Ball Demo Start");
    system_api.set_render_callback(render);
    
    while (1) {
        // Move the ball
        x += dx;
        y += dy;
        
        // Bounce off walls
        if (x <= 0 || x >= 123) dx = -dx;
        if (y <= 11 || y >= 59) dy = -dy;
        
        // Non-interactive exit check (if events worked)
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN && (e.data1 & BTN_B)) {
                ((void (*)(void))0x0000)();
            }
        }
        
        system_api.lcd_update();
        system_api.wait_tick();
    }
    
    return 0;
}
