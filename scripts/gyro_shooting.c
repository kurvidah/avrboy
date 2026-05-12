#include "avrboy.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define A system_api
#define SCREEN_W 128
#define SCREEN_H 64

/* Game States */
#define ST_TITLE 0
#define ST_PLAY  1
#define ST_OVER  2

/* Settings */
#define MAX_TARGETS 4
#define GAME_TICKS 1800 // 30 seconds at 60fps

typedef struct {
    int16_t x, y;
    int8_t speed;
    bool active;
    uint8_t size;
} Target;

/* Global State */
static Target targets[MAX_TARGETS];
static int16_t cross_x = 64, cross_y = 32;
static uint16_t score = 0;
static uint16_t ticks_left = 0;
static uint8_t state = ST_TITLE;
static uint8_t flash_timer = 0;
static uint8_t beep_timer = 0; 
static mpu_data_t mpu;

/* --- Logic Helpers --- */

static void spawn_target(uint8_t i) {
    targets[i].y = 10 + (rand() % 40);
    targets[i].speed = (rand() % 3) + 1;
    targets[i].size = 6 + (rand() % 5);
    
    if (rand() % 2 == 0) {
        targets[i].x = -15; 
    } else {
        targets[i].x = 135; 
        targets[i].speed = -targets[i].speed; 
    }
    targets[i].active = true;
}

static void init_game() {
    score = 0;
    ticks_left = GAME_TICKS;
    A.play_tone(0); 
    beep_timer = 0;
    
    for (uint8_t i = 0; i < MAX_TARGETS; i++) {
        spawn_target(i);
        targets[i].x = rand() % 120; 
    }
    state = ST_PLAY;
}

static void shoot() {
    if (state != ST_PLAY) return;
    
    flash_timer = 3;   

    for (uint8_t i = 0; i < MAX_TARGETS; i++) {
        if (targets[i].active) {
            // Check if crosshair is inside target bounding box
            if (cross_x >= targets[i].x && cross_x <= targets[i].x + targets[i].size &&
                cross_y >= targets[i].y && cross_y <= targets[i].y + targets[i].size) {
                
                targets[i].active = false;
                score++;
                
                // Beep only on hit
                A.play_tone(2000); 
                beep_timer = 10; 
            }
        }
    }
}

static void update_play() {
    A.mpu_read(&mpu);
    
    // Map Gyro to Screen
    cross_x = 64 + (mpu.gx / 250); 
    cross_y = 32 + (mpu.gy / 500); 

    if (cross_x < 0) cross_x = 0; 
    if (cross_x > 127) cross_x = 127;
    if (cross_y < 0) cross_y = 0; 
    if (cross_y > 63) cross_y = 63;

    // Move Targets
    for (uint8_t i = 0; i < MAX_TARGETS; i++) {
        if (targets[i].active) {
            targets[i].x += targets[i].speed;
            if (targets[i].x < -20 || targets[i].x > 148) {
                spawn_target(i);
            }
        } else {
            spawn_target(i); 
        }
    }

    if (flash_timer > 0) flash_timer--;
    
    if (beep_timer > 0) {
        if (--beep_timer == 0) A.play_tone(0); 
    }
    
    if (ticks_left > 0) {
        ticks_left--;
    } else {
        state = ST_OVER;
        A.play_tone(150); 
        beep_timer = 30; 
    }
}

/* --- Rendering --- */

static void render() {
    char buf[32];

    if (state == ST_TITLE) {
        A.draw_string(14, 20, "SHOOTING GALLERY", 1);
        A.draw_string(20, 40, "PRESS B TO START", 1);
    } 
    else if (state == ST_PLAY) {
        if (flash_timer > 0) {
            A.fill_rect(0, 0, 128, 64, 1);
            return; 
        }

        for (uint8_t i = 0; i < MAX_TARGETS; i++) {
            if (targets[i].active) {
                int16_t tx = targets[i].x;
                int16_t ty = targets[i].y;
                int8_t s = targets[i].size;

                A.draw_line(tx, ty, tx + s, ty, 1);       
                A.draw_line(tx, ty + s, tx + s, ty + s, 1); 
                A.draw_line(tx, ty, tx, ty + s, 1);       
                A.draw_line(tx + s, ty, tx + s, ty + s, 1); 
                A.draw_pixel(tx + (s/2), ty + (s/2), 1);  
            }
        }

        // Crosshair
        A.draw_line(cross_x - 5, cross_y, cross_x + 5, cross_y, 1);
        A.draw_line(cross_x, cross_y - 5, cross_x, cross_y + 5, 1);
        
        sprintf(buf, "SCORE:%d", score);
        A.draw_string(2, 2, buf, 1);

        sprintf(buf, "TIME:%d", ticks_left / 60);
        A.draw_string(85, 2, buf, 1);
    } 
    else if (state == ST_OVER) {
        A.draw_string(35, 10, "TIME'S UP!", 1);
        sprintf(buf, "FINAL SCORE: %d", score);
        A.draw_string(20, 30, buf, 1);
        A.draw_string(15, 50, "PRESS B TO REPLAY", 1);
    }
}

/* --- Entry Point --- */

APP_MAIN() {
    A.set_render_callback(render);
    
    bool b_held = false; 
    
    while(1) {
        Event e;
        while(A.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if (e.data1 & BTN_B) {
                    if (!b_held) {
                        b_held = true; 
                        if (state == ST_TITLE || state == ST_OVER) {
                            init_game();
                        } else if (state == ST_PLAY) {
                            shoot();
                        }
                    }
                }
                
                // Reset/Exit shortcut
                if ((e.data1 & (BTN_A | BTN_B)) == (BTN_A | BTN_B)) {
                    ((void (*)(void))0x0000)();
                }
            }
            else if (e.type == EVENT_BTN_UP) {
                // Unlock latch immediately on any release event
                b_held = false; 
            }
        }
        
        if (state == ST_PLAY) {
            update_play();
        } else if (state == ST_OVER && beep_timer > 0) {
            if (--beep_timer == 0) A.play_tone(0);
        }
        
        A.lcd_update();
        A.wait_tick();
    }
    return 0;
}