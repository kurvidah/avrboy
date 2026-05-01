#include "avrboy.h"

static uint8_t jx = 128, jy = 128;
static uint8_t btns = 0;

void draw_btn(uint8_t x, uint8_t y, const char* label, uint8_t mask) {
    bool pressed = btns & mask;
    if (pressed) {
        system_api.fill_rect(x-1, y-1, 10, 10, 1);
        system_api.draw_string(x+1, y, label, 0);
    } else {
        // Draw outline
        system_api.draw_line(x-1, y-1, x+8, y-1, 1);
        system_api.draw_line(x-1, y+8, x+8, y+8, 1);
        system_api.draw_line(x-1, y-1, x-1, y+8, 1);
        system_api.draw_line(x+8, y-1, x+8, y+8, 1);
        system_api.draw_string(x+1, y, label, 1);
    }
}

void render(void) {
    system_api.draw_string(2, 1, "JOYSTICK VISUAL", 1);
    system_api.draw_line(0, 10, 127, 10, 1);
    
    // 1. Draw Joystick Box (Centered)
    system_api.draw_line(44, 17, 84, 17, 1);
    system_api.draw_line(44, 57, 84, 57, 1);
    system_api.draw_line(44, 17, 44, 57, 1);
    system_api.draw_line(84, 17, 84, 57, 1);
    
    int16_t offsetX = ((int16_t)jx - 128) * 20 / 128;
    int16_t offsetY = ((int16_t)jy - 128) * 20 / 128;
    system_api.fill_rect(64 + offsetX - 2, 37 + offsetY - 2, 5, 5, 1);
    
    // 2. Button Indicators
    draw_btn(15, 20, "U", BTN_UP);
    draw_btn(15, 40, "D", BTN_DOWN);
    draw_btn(5,  30, "L", BTN_LEFT);
    draw_btn(25, 30, "R", BTN_RIGHT);
    
    draw_btn(100, 25, "A", BTN_A);
    draw_btn(115, 35, "B", BTN_B);

    system_api.draw_string(2, 55, "A+B TO EXIT", 1);
}

APP_MAIN() {
    system_api.log("Joy Visualizer Active");
    system_api.set_render_callback(render);
    
    while (1) {
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_JOY_MOVE) {
                jx = e.data1;
                jy = e.data2;
            } else if (e.type == EVENT_BTN_DOWN) {
                btns = e.data1;
                
                // Play specific tones for each button
                if (btns & BTN_UP)         system_api.play_tone(440); // A4
                else if (btns & BTN_DOWN)  system_api.play_tone(392); // G4
                else if (btns & BTN_LEFT)  system_api.play_tone(349); // F4
                else if (btns & BTN_RIGHT) system_api.play_tone(330); // E4
                else if (btns & BTN_A)     system_api.play_tone(523); // C5
                else if (btns & BTN_B)     system_api.play_tone(587); // D5

                // Exit combo
                if ((btns & (BTN_A | BTN_B)) == (BTN_A | BTN_B)) {
                    system_api.play_tone(0);
                    ((void (*)(void))0x0000)();
                }
            } else if (e.type == EVENT_BTN_UP) {
                btns = e.data1;
                if (btns == 0) system_api.play_tone(0); // Stop if all released
            }
        }
        
        system_api.lcd_update();
        system_api.wait_tick();
    }
}
