#include "avrboy.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* --- Game Settings --- */
#define A system_api
#define MAX_LEN 512  /* 32 * 16 grid cells */
#define GRID_W 32
#define GRID_H 16
#define CELL 4

/* CONSTANT SPEED SETTING */
#define SNAKE_SPEED 4 

/* --- Structures --- */
typedef struct {
    int8_t x, y;
} Point;

/* --- Game State --- */
static Point snake[MAX_LEN];
static int16_t snk_len;
static int8_t dx, dy;
static Point food;

static uint8_t st = 0; // 0: Title, 1: Playing, 2: Game Over, 3: Win
static uint16_t score = 0;
static uint16_t tk = 0;
static uint8_t at = 0; // Audio timer

/* --- Helpers --- */
static void spawn_food() {
    // If the snake occupies every cell, there is no room for food
    if (snk_len >= MAX_LEN) return;

    bool ok;
    do {
        ok = true;
        food.x = rand() % GRID_W;
        food.y = rand() % GRID_H;
        
        for (int i = 0; i < snk_len; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                ok = false;
                break;
            }
        }
    } while (!ok);
}

static void rs() {
    snk_len = 4;
    snake[0].x = 16; snake[0].y = 8;
    snake[1].x = 15; snake[1].y = 8;
    snake[2].x = 14; snake[2].y = 8;
    snake[3].x = 13; snake[3].y = 8;
    
    dx = 1; dy = 0;
    score = 0;
    spawn_food();
}

/* --- Logic --- */
static void up() {
    if (st != 1) return;
    tk++;

    if (tk % SNAKE_SPEED != 0) return; 

    // 1. Move Body
    for (int i = snk_len - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    
    // 2. Move Head
    snake[0].x += dx;
    snake[0].y += dy;

    // 3. WALL PORTALS
    if (snake[0].x < 0) snake[0].x = GRID_W - 1;
    else if (snake[0].x >= GRID_W) snake[0].x = 0;

    if (snake[0].y < 0) snake[0].y = GRID_H - 1;
    else if (snake[0].y >= GRID_H) snake[0].y = 0;

    // 4. Collision: Self
    for (int i = 1; i < snk_len; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            st = 2; 
            A.play_tone(150); at = 30;
            return;
        }
    }

    // 5. Check Food / Win Condition
    if (snake[0].x == food.x && snake[0].y == food.y) {
        score++;
        A.play_tone(800); at = 10;
        
        if (snk_len < MAX_LEN) {
            snk_len++;
            // If the snake now fills all cells, player wins
            if (snk_len == MAX_LEN) {
                st = 3; 
                A.play_tone(1200); at = 40;
            } else {
                spawn_food();
            }
        }
    }
}

/* --- Rendering --- */
static void rd() {
    if (st == 0) {
        A.draw_string(20, 20, "HOLLOW SNAKE", 1);
        A.draw_string(20, 34, "PRESS B TO START", 1);
    } 
    else {
        // Draw Food (only if playing)
        if (st == 1) {
            A.fill_rect(food.x * CELL, food.y * CELL, CELL, CELL, 1);
        }
        
        // Pass A: Draw solid boxes
        for (int i = 0; i < snk_len; i++) {
            A.fill_rect(snake[i].x * CELL, snake[i].y * CELL, CELL, CELL, 1);
        }
        
        // Pass B: Carve centers
        for (int i = 0; i < snk_len; i++) {
            int px = snake[i].x * CELL;
            int py = snake[i].y * CELL;
            A.fill_rect(px + 1, py + 1, CELL - 2, CELL - 2, 0);
            
            if (i < snk_len - 1) {
                int nx = snake[i+1].x * CELL;
                int ny = snake[i+1].y * CELL;
                int diff_x = nx - px;
                int diff_y = ny - py;

                if (diff_x == CELL || diff_x < -CELL)      A.fill_rect(px + 3, py + 1, 2, 2, 0); 
                else if (diff_x == -CELL || diff_x > CELL) A.fill_rect(px - 1, py + 1, 2, 2, 0); 
                else if (diff_y == CELL || diff_y < -CELL) A.fill_rect(px + 1, py + 3, 2, 2, 0); 
                else if (diff_y == -CELL || diff_y > CELL) A.fill_rect(px + 1, py - 1, 2, 2, 0);
            }
        }

        // Win/Loss Overlays
        if (st == 2 || st == 3) {
            A.fill_rect(24, 20, 80, 24, 0);
            A.draw_line(24, 20, 104, 20, 1);
            A.draw_line(24, 44, 104, 44, 1);
            
            if (st == 2) A.draw_string(34, 24, "GAME OVER", 1);
            else        A.draw_string(38, 24, "YOU WIN!", 1);
            
            A.draw_string(28, 34, "B TO RESTART", 1);
        }
    }
}

/* --- Entry Point --- */
APP_MAIN() {
    A.set_render_callback(rd);
    rs();

    while (1) {
        Event e;
        while (A.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if ((st == 0 || st == 2 || st == 3) && (e.data1 & BTN_B)) { 
                    rs(); 
                    st = 1; 
                }
                
                if (st == 1) {
                    if ((e.data1 & BTN_UP) && dy != 1)    { dx = 0; dy = -1; }
                    if ((e.data1 & BTN_DOWN) && dy != -1) { dx = 0; dy = 1; }
                    if ((e.data1 & BTN_LEFT) && dx != 1)  { dx = -1; dy = 0; }
                    if ((e.data1 & BTN_RIGHT) && dx != -1) { dx = 1; dy = 0; }
                }
            }
        }

        if (at > 0 && --at == 0) A.play_tone(0);
        
        up();
        A.lcd_update();
        A.wait_tick();
    }
    return 0;
}