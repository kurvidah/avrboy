#include "avrboy.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Game Settings
#define A system_api
#define W 128
#define H 64
#define NUM_LEVELS 5

// Maze Map Definitions (16x8 grid, each cell is 8x8 pixels)
// 0 = Empty, 1 = Wall, 2 = Hole, 3 = Goal, 4 = Start
static const uint8_t levels[NUM_LEVELS][8][16] = {
    { // Level 1 - The Basics
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,4,0,0,0,0,1,3,0,0,1,0,0,2,0,1},
        {1,0,1,1,1,0,1,1,1,0,1,0,1,1,0,1},
        {1,0,1,2,0,0,0,0,1,0,0,0,0,1,0,1},
        {1,0,1,1,1,1,1,0,1,1,1,1,0,1,0,1},
        {1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Level 2 - The Corners
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,4,0,0,0,1,0,0,0,0,0,0,0,0,2,1},
        {1,1,1,1,0,1,0,1,1,1,1,1,1,0,1,1},
        {1,2,0,0,0,0,0,1,3,0,0,0,1,0,0,1},
        {1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1},
        {1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Level 3 - The Spiral
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,4,0,0,0,0,0,0,0,0,0,0,0,0,2,1},
        {1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
        {1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,1},
        {1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1},
        {1,0,1,0,1,3,0,0,0,0,1,0,1,0,1,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Level 4 - The Weave
        {1,1,1,2,1,2,1,2,1,2,1,2,1,2,1,1},
        {1,4,1,0,0,0,1,0,0,0,1,0,0,0,1,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
        {1,0,0,0,1,0,0,0,1,0,0,0,1,3,1,1},
        {1,2,1,2,1,2,1,2,1,2,1,2,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    { // Level 5 - Final Test
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,4,0,1,2,0,0,0,0,0,0,3,0,0,2,1},
        {1,1,0,1,1,1,1,1,1,0,1,1,1,0,1,1},
        {1,2,0,0,0,0,0,0,1,0,0,0,1,0,1,1},
        {1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,1},
        {1,0,0,0,0,0,1,0,0,0,2,0,0,0,2,1},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    }
};

static int32_t px, py, vx, vy;
static int8_t ax, ay;
static uint8_t st = 0, lv = 0, at = 0;
static uint16_t tk = 0;

static bool hit_wall(int x, int y) {
    int cx1 = x / 8, cx2 = (x + 3) / 8, cy1 = y / 8, cy2 = (y + 3) / 8;
    if (cx1 < 0 || cx2 > 15 || cy1 < 0 || cy2 > 7) return true;
    return (levels[lv][cy1][cx1] == 1 || levels[lv][cy1][cx2] == 1 || levels[lv][cy2][cx1] == 1 || levels[lv][cy2][cx2] == 1);
}

static int get_center_cell(int x, int y) {
    int cx = (x + 1) / 8, cy = (y + 1) / 8;
    return (cx < 0 || cx > 15 || cy < 0 || cy > 7) ? 0 : levels[lv][cy][cx];
}

static void rs(bool full_reset) {
    if (full_reset) lv = 0;
    vx = 0; vy = 0; ax = 0; ay = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 16; c++) {
            if (levels[lv][r][c] == 4) { px = (c * 8 + 2) << 4; py = (r * 8 + 2) << 4; }
        }
    }
}

static void up() {
    if (st != 1) return;
    tk++;
    vx += ax * 2; vy += ay * 2;
    vx -= vx / 16; vy -= vy / 16;
    px += vx;
    if (hit_wall(px >> 4, py >> 4)) { px -= vx; vx = -(vx * 3) / 4; A.play_tone(200); at = 1; }
    py += vy;
    if (hit_wall(px >> 4, py >> 4)) { py -= vy; vy = -(vy * 3) / 4; A.play_tone(200); at = 1; }

    int center = get_center_cell(px >> 4, py >> 4);
    if (center == 2) { st = 2; A.play_tone(100); at = 25; }
    else if (center == 3) {
        lv++;
        if (lv >= NUM_LEVELS) { st = 3; A.play_tone(2000); at = 40; }
        else { rs(false); A.play_tone(800); at = 15; st = 0; }
    }
}

static void rd() {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 16; c++) {
            uint8_t cell = levels[lv][r][c];
            int x = c * 8, y = r * 8;
            if (cell == 1) { A.fill_rect(x, y, 8, 8, 1); A.draw_pixel(x + 2, y + 2, 0); A.draw_pixel(x + 5, y + 5, 0); } 
            else if (cell == 2) { A.draw_line(x+2, y+1, x+5, y+1, 1); A.draw_line(x+2, y+6, x+5, y+6, 1); A.draw_line(x+1, y+2, x+1, y+5, 1); A.draw_line(x+6, y+2, x+6, y+5, 1); }
            else if (cell == 3) { if ((tk / 4) % 2 == 0) A.fill_rect(x + 2, y + 2, 4, 4, 1); else A.draw_line(x+2, y+2, x+5, y+5, 1); }
        }
    }
    if (st == 1) { int sx = px >> 4, sy = py >> 4; A.fill_rect(sx, sy, 4, 4, 1); A.draw_pixel(sx + 1, sy + 1, 0); }
    if (st == 0) {
        A.fill_rect(10, 20, 108, 24, 0); A.draw_line(10, 20, 118, 20, 1); A.draw_line(10, 44, 118, 44, 1);
        if (lv == 0) A.draw_string(16, 24, "MARBLE LABYRINTH", 1);
        else { char l_str[] = "LEVEL 0"; l_str[6] = '1' + lv; A.draw_string(40, 24, l_str, 1); }
        A.draw_string(16, 34, "PRESS B TO START", 1);
    } 
    else if (st == 2) { A.fill_rect(20, 24, 88, 16, 0); A.draw_string(34, 28, "DIED! B:RESPAWN", 1); }
    else if (st == 3) { A.fill_rect(15, 20, 98, 24, 0); A.draw_string(40, 24, "YOU WIN!", 1); A.draw_string(14, 34, "RESTART PRESS B", 1); }
}

APP_MAIN() {
    A.set_render_callback(rd); rs(true);
    while (1) {
        Event e;
        while (A.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN && (e.data1 & BTN_B)) {
                if (st == 0) st = 1;
                else if (st == 2) { rs(false); st = 1; }
                else if (st == 3) { rs(true); st = 0; }
            }
            if (e.type == EVENT_JOY_MOVE && st == 1) {
                ax = (e.data1 > 150 || (int8_t)e.data1 > 30) ? 1 : ((e.data1 > 0 && e.data1 < 100) || (int8_t)e.data1 < -30) ? -1 : 0;
                ay = (e.data2 > 150 || (int8_t)e.data2 > 30) ? 1 : ((e.data2 > 0 && e.data2 < 100) || (int8_t)e.data2 < -30) ? -1 : 0;
            }
        }
        if (at > 0 && --at == 0) A.play_tone(0);
        up(); A.lcd_update(); A.wait_tick();
    }
    return 0;
}