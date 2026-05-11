#include "avrboy.h"
#include <stdio.h>

static mpu_data_t mpu;

void render(void) {
    char buf[32];
    
    system_api.draw_string(2, 2, "MPU6050 SIM (DIP-28)", 1);
    system_api.draw_line(0, 10, 127, 10, 1);
    
    // Display Absolute Angle (GX/GY)
    system_api.draw_string(2, 14, "ANGLE (GX/GY):", 1);
    sprintf(buf, "X:%d", mpu.gx);
    system_api.draw_string(10, 22, buf, 1);
    sprintf(buf, "Y:%d", mpu.gy);
    system_api.draw_string(70, 22, buf, 1);
    
    // Display Acceleration (AX/AY/AZ)
    system_api.draw_string(2, 32, "ACCEL (X/Y/Z):", 1);
    sprintf(buf, "%d/%d/%d", mpu.ax, mpu.ay, mpu.az);
    system_api.draw_string(10, 40, buf, 1);
    
    // Draw "Level" bubble based on absolute angle (GX/GY)
    // Box: 44-84 (width 40, ctr 64), 46-62 (height 16, ctr 54)
    // gx/gy range +/- 18000
    int16_t bx = 64 + (mpu.gx / 900); // 18000 / 20 = 900
    int16_t by = 54 + (mpu.gy / 2250); // 18000 / 8 = 2250
    
    // Boundary box for level
    system_api.draw_line(44, 46, 84, 46, 1);
    system_api.draw_line(44, 62, 84, 62, 1);
    system_api.draw_line(44, 46, 44, 62, 1);
    system_api.draw_line(84, 46, 84, 62, 1);
    
    system_api.fill_rect(bx - 2, by - 2, 5, 5, 1);
    
    if (mpu.gx == 0 && mpu.gy == 0) {
        system_api.draw_string(90, 50, "LEVEL!", 1);
    }
}

APP_MAIN() {
    system_api.set_render_callback(render);
    
    while(1) {
        system_api.mpu_read(&mpu);
        
        Event e;
        while(system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN && (e.data1 & (BTN_A | BTN_B)) == (BTN_A | BTN_B)) {
                ((void (*)(void))0x0000)();
            }
        }
        
        system_api.lcd_update();
        system_api.wait_tick();
    }
}
