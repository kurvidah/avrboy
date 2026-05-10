#include "avrboy.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* --- Game Constants --- */
#define TILE_SIZE 4
#define MAP_COLS 32
#define MAP_ROWS 14
#define Y_OFFSET 8

#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

#define STATE_TITLE 0
#define STATE_PLAY  1
#define STATE_OVER  2
#define STATE_WIN   3

#define GHOST_NORMAL     0
#define GHOST_FRIGHTENED 1
#define GHOST_DEAD       2

/* --- Global State --- */
static uint8_t game_state = STATE_TITLE;
static uint16_t score = 0;
static int8_t lives = 3;
static uint16_t dots_remaining = 0;
static uint16_t dots_eaten = 0;
static uint8_t audio_timer = 0;
static uint16_t tick_counter = 0;
static int16_t frightened_ticks = 0;
static bool fruit_spawned = false;

/* --- Custom itoa --- */
static void itoa_custom(uint16_t value, char *str) {
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int i = 0;
    while (value > 0) {
        str[i++] = (value % 10) + '0';
        value /= 10;
    }
    str[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/* --- Map Data --- */
// 0: Empty, 1: Wall, 2: Dot, 3: Power Pellet, 4: Fruit, 5: Ghost Door
static uint8_t map[MAP_ROWS][MAP_COLS];

static void generate_map(void) {
    dots_remaining = 0;
    dots_eaten = 0;
    fruit_spawned = false;
    
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            if (y == 0 || y == MAP_ROWS - 1 || x == 0 || x == MAP_COLS - 1) {
                if ((y == 6 || y == 7) && (x == 0 || x == MAP_COLS - 1)) {
                    map[y][x] = 0; // Tunnels
                } else {
                    map[y][x] = 1;
                }
            } else if (y >= 5 && y <= 8 && x >= 13 && x <= 18) {
                map[y][x] = (y == 5 || y == 8 || x == 13 || x == 18) ? 1 : 0;
                if (y == 5 && (x == 15 || x == 16)) map[y][x] = 5; // Door
            } else if (y % 2 == 0 && x % 4 == 0) {
                map[y][x] = 1;
                map[y][x+1] = 1;
            } else {
                map[y][x] = 2;
            }
        }
    }
    
    // Clear start areas & set Power Pellets
    map[10][15] = 0; map[10][16] = 0;
    map[2][2] = 3; map[2][MAP_COLS-3] = 3;
    map[MAP_ROWS-3][2] = 3; map[MAP_ROWS-3][MAP_COLS-3] = 3;

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            if (map[y][x] == 2 || map[y][x] == 3) dots_remaining++;
        }
    }
}

/* --- Entities --- */
typedef struct {
    int16_t x, y;
    uint8_t dir;
    uint8_t next_dir;
} Player;

typedef struct {
    int16_t x, y;
    uint8_t dir;
    uint8_t state;
    uint8_t respawn_timer;
} Ghost;

static Player player;
#define MAX_GHOSTS 3
static Ghost ghosts[MAX_GHOSTS];

static void reset_entities(void) {
    player.x = 15 * TILE_SIZE;
    player.y = 10 * TILE_SIZE;
    player.dir = DIR_LEFT;
    player.next_dir = DIR_LEFT;
    
    for (int i = 0; i < MAX_GHOSTS; i++) {
        ghosts[i].x = (14 + i) * TILE_SIZE;
        ghosts[i].y = 7 * TILE_SIZE;
        ghosts[i].dir = DIR_UP;
        ghosts[i].state = GHOST_NORMAL;
        ghosts[i].respawn_timer = i * 10; // Stagger exits
    }
    frightened_ticks = 0;
}

static void init_game(void) {
    score = 0;
    lives = 3;
    generate_map();
    reset_entities();
    game_state = STATE_PLAY;
}

/* --- Logic Helpers --- */
static bool is_aligned(int16_t x, int16_t y) {
    return (x % TILE_SIZE == 0) && (y % TILE_SIZE == 0);
}

static bool can_move(int16_t x, int16_t y, uint8_t dir, bool is_ghost) {
    int16_t tx = x / TILE_SIZE;
    int16_t ty = y / TILE_SIZE;
    
    if (dir == DIR_UP) ty -= 1;
    else if (dir == DIR_DOWN) ty += 1;
    else if (dir == DIR_LEFT) tx -= 1;
    else if (dir == DIR_RIGHT) tx += 1;
    
    if (tx < 0) tx = MAP_COLS - 1;
    if (tx >= MAP_COLS) tx = 0;
    
    if (map[ty][tx] == 1) return false;
    if (!is_ghost && map[ty][tx] == 5) return false; // Player can't enter door
    
    return true;
}

static uint8_t get_opposite_dir(uint8_t dir) {
    if (dir == DIR_UP) return DIR_DOWN;
    if (dir == DIR_DOWN) return DIR_UP;
    if (dir == DIR_LEFT) return DIR_RIGHT;
    if (dir == DIR_RIGHT) return DIR_LEFT;
    return DIR_NONE;
}

static int16_t abs_val(int16_t v) { return v < 0 ? -v : v; }

/* --- Game Update Loop --- */
static void update(void) {
    if (game_state != STATE_PLAY) return;
    tick_counter++;

    // Frightened Timer Logic
    if (frightened_ticks > 0) {
        frightened_ticks--;
        if (frightened_ticks == 0) {
            for (int i = 0; i < MAX_GHOSTS; i++) {
                if (ghosts[i].state == GHOST_FRIGHTENED) ghosts[i].state = GHOST_NORMAL;
            }
        }
    }

    // Player Movement
    if (player.x < 0) player.x = (MAP_COLS * TILE_SIZE) - 1;
    if (player.x >= MAP_COLS * TILE_SIZE) player.x = 0;

    if (is_aligned(player.x, player.y)) {
        if (player.next_dir != DIR_NONE && can_move(player.x, player.y, player.next_dir, false)) {
            player.dir = player.next_dir;
        }
    }
    
    if (can_move(player.x, player.y, player.dir, false) || !is_aligned(player.x, player.y)) {
        if (player.dir == DIR_UP) player.y -= 1;
        else if (player.dir == DIR_DOWN) player.y += 1;
        else if (player.dir == DIR_LEFT) player.x -= 1;
        else if (player.dir == DIR_RIGHT) player.x += 1;
    }

    // Eat dots, pellets, and fruit
    if (is_aligned(player.x, player.y)) {
        int tx = player.x / TILE_SIZE;
        int ty = player.y / TILE_SIZE;
        uint8_t tile = map[ty][tx];
        
        if (tile == 2 || tile == 3 || tile == 4) {
            map[ty][tx] = 0;
            
            if (tile == 2) {
                score += 10;
                dots_remaining--;
                dots_eaten++;
                system_api.play_tone(2000); audio_timer = 2;
            } else if (tile == 3) { // Power Pellet
                score += 50;
                dots_remaining--;
                dots_eaten++;
                frightened_ticks = 200; // Frightened duration
                system_api.play_tone(1500); audio_timer = 10;
                
                // Gimmick: Reverse ghost directions on pellet eat
                for (int i = 0; i < MAX_GHOSTS; i++) {
                    if (ghosts[i].state == GHOST_NORMAL || ghosts[i].state == GHOST_FRIGHTENED) {
                        ghosts[i].state = GHOST_FRIGHTENED;
                        if (is_aligned(ghosts[i].x, ghosts[i].y)) {
                            ghosts[i].dir = get_opposite_dir(ghosts[i].dir);
                        }
                    }
                }
            } else if (tile == 4) { // Fruit
                score += 100;
                system_api.play_tone(3500); audio_timer = 15;
            }

            // Gimmick: Spawn Fruit at 30 dots
            if (dots_eaten == 30 && !fruit_spawned) {
                map[9][15] = 4;
                fruit_spawned = true;
            }

            if (dots_remaining == 0) {
                system_api.play_tone(3000); audio_timer = 30;
                game_state = STATE_WIN;
            }
        }
    }

    // Ghost Movement & AI
    for (int i = 0; i < MAX_GHOSTS; i++) {
        Ghost *g = &ghosts[i];
        
        // Timing logic: Normal = every 2nd tick, Frightened = every 3rd tick, Dead = every tick (fast)
        uint8_t move_mod = (g->state == GHOST_DEAD) ? 1 : (g->state == GHOST_FRIGHTENED ? 3 : 2);
        if (tick_counter % move_mod != 0) continue;

        if (g->state == GHOST_DEAD) {
            if (g->respawn_timer > 0) g->respawn_timer--;
            else {
                g->state = GHOST_NORMAL;
                g->dir = DIR_UP; // Exit door
            }
            continue;
        }

        // Wrap around
        if (g->x < 0) g->x = (MAP_COLS * TILE_SIZE) - 1;
        if (g->x >= MAP_COLS * TILE_SIZE) g->x = 0;

        if (is_aligned(g->x, g->y)) {
            uint8_t possible[4];
            uint8_t num_dirs = 0;
            uint8_t rev = get_opposite_dir(g->dir);
            
            for (uint8_t d = 1; d <= 4; d++) {
                if (d != rev && can_move(g->x, g->y, d, true)) {
                    possible[num_dirs++] = d;
                }
            }

            if (num_dirs > 0) {
                if (i == 0 && g->state == GHOST_NORMAL && num_dirs > 1) {
                    // Blinky AI: Pick direction that minimizes distance to player
                    uint8_t best_dir = possible[0];
                    int16_t min_dist = 9999;
                    for (int j = 0; j < num_dirs; j++) {
                        int16_t test_x = g->x; int16_t test_y = g->y;
                        if (possible[j] == DIR_UP) test_y -= TILE_SIZE;
                        else if (possible[j] == DIR_DOWN) test_y += TILE_SIZE;
                        else if (possible[j] == DIR_LEFT) test_x -= TILE_SIZE;
                        else if (possible[j] == DIR_RIGHT) test_x += TILE_SIZE;
                        
                        int16_t dist = abs_val(test_x - player.x) + abs_val(test_y - player.y);
                        if (dist < min_dist) { min_dist = dist; best_dir = possible[j]; }
                    }
                    g->dir = best_dir;
                } else {
                    // Random / Frightened scatter
                    g->dir = possible[(player.x + g->y + tick_counter + i) % num_dirs];
                }
            } else {
                g->dir = rev; // Dead end, must reverse
            }
        }
        
        if (g->dir == DIR_UP) g->y -= 1;
        else if (g->dir == DIR_DOWN) g->y += 1;
        else if (g->dir == DIR_LEFT) g->x -= 1;
        else if (g->dir == DIR_RIGHT) g->x += 1;
        
        // Collision
        if (abs_val(player.x - g->x) < TILE_SIZE - 1 && abs_val(player.y - g->y) < TILE_SIZE - 1) {
            if (g->state == GHOST_FRIGHTENED) {
                // Eat ghost
                score += 200;
                g->state = GHOST_DEAD;
                g->x = 15 * TILE_SIZE;
                g->y = 7 * TILE_SIZE; // Send to ghost house
                g->respawn_timer = 50;
                system_api.play_tone(4000); audio_timer = 10;
            } else if (g->state == GHOST_NORMAL) {
                // Player dies
                lives--;
                system_api.play_tone(300); audio_timer = 20;
                if (lives > 0) reset_entities();
                else game_state = STATE_OVER;
            }
        }
    }
}

/* --- Rendering --- */
static void render(void) {
    // 1. Draw Map
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLS; x++) {
            uint8_t px = x * TILE_SIZE;
            uint8_t py = y * TILE_SIZE + Y_OFFSET;
            
            if (map[y][x] == 1) {
                system_api.draw_line(px, py, px+TILE_SIZE-1, py, 1);
                system_api.draw_line(px, py, px, py+TILE_SIZE-1, 1);
            } else if (map[y][x] == 2) {
                system_api.draw_pixel(px + 1, py + 1, 1);
            } else if (map[y][x] == 3) {
                if (tick_counter % 10 < 5) system_api.fill_rect(px+1, py+1, 2, 2, 1); // Blinking Pellet
            } else if (map[y][x] == 4) {
                // Fruit (Cherry-like)
                system_api.fill_rect(px+1, py+2, 2, 2, 1);
                system_api.draw_pixel(px+2, py+1, 1); // Stem
            } else if (map[y][x] == 5) {
                system_api.draw_line(px, py+2, px+TILE_SIZE-1, py+2, 1); // Door
            }
        }
    }

    // 2. Draw Player
    if (game_state == STATE_PLAY || (tick_counter % 10 < 5)) {
        uint8_t px = player.x;
        uint8_t py = player.y + Y_OFFSET;
        system_api.fill_rect(px, py, TILE_SIZE, TILE_SIZE, 1);
        
        if (tick_counter % 8 < 4) { // Mouth open
            if (player.dir == DIR_RIGHT) system_api.fill_rect(px+2, py+1, 2, 2, 0);
            if (player.dir == DIR_LEFT)  system_api.fill_rect(px,   py+1, 2, 2, 0);
            if (player.dir == DIR_UP)    system_api.fill_rect(px+1, py,   2, 2, 0);
            if (player.dir == DIR_DOWN)  system_api.fill_rect(px+1, py+2, 2, 2, 0);
        }
    }

    // 3. Draw Ghosts
    for (int i = 0; i < MAX_GHOSTS; i++) {
        uint8_t gx = ghosts[i].x;
        uint8_t gy = ghosts[i].y + Y_OFFSET;
        
        if (ghosts[i].state == GHOST_NORMAL) {
            system_api.fill_rect(gx, gy, TILE_SIZE, TILE_SIZE, 1);
            system_api.draw_pixel(gx+1, gy+1, 0); // L-Eye
            system_api.draw_pixel(gx+3, gy+1, 0); // R-Eye
        } else if (ghosts[i].state == GHOST_FRIGHTENED) {
            // Hollow Wavy Box for Frightened
            system_api.draw_line(gx, gy, gx+TILE_SIZE-1, gy, 1);
            system_api.draw_line(gx, gy+TILE_SIZE-1, gx+TILE_SIZE-1, gy+TILE_SIZE-1, 1);
            system_api.draw_line(gx, gy, gx, gy+TILE_SIZE-1, 1);
            system_api.draw_line(gx+TILE_SIZE-1, gy, gx+TILE_SIZE-1, gy+TILE_SIZE-1, 1);
            if (frightened_ticks > 30 || tick_counter % 4 < 2) { // Blink when wearing off
                 system_api.draw_pixel(gx+1, gy+2, 1); // Sad mouth
                 system_api.draw_pixel(gx+2, gy+2, 1);
            }
        } else if (ghosts[i].state == GHOST_DEAD) {
            // Just Floating Eyes
            system_api.draw_pixel(gx+1, gy+1, 1);
            system_api.draw_pixel(gx+3, gy+1, 1);
        }
    }

    // 4. Header UI
    char str_buf[8];
    system_api.draw_string(0, 0, "LIFE:", 1);
    for (int i = 0; i < lives; i++) {
        system_api.fill_rect(30 + (i * 6), 1, 4, 4, 1);
    }
    
    system_api.draw_string(70, 0, "SCORE:", 1);
    itoa_custom(score, str_buf);
    system_api.draw_string(106, 0, str_buf, 1);

    // Context Overlays
    if (game_state == STATE_TITLE) {
        system_api.fill_rect(20, 24, 88, 16, 0);
        system_api.draw_string(24, 28, "PRESS B TO START", 1);
    } else if (game_state == STATE_OVER) {
        system_api.fill_rect(30, 24, 68, 16, 0);
        system_api.draw_string(34, 28, "GAME OVER", 1);
    } else if (game_state == STATE_WIN) {
        system_api.fill_rect(30, 24, 68, 16, 0);
        system_api.draw_string(36, 28, "YOU WIN!", 1);
    }
}

APP_MAIN() {
    system_api.log("Pac-Man Extended Start");
    system_api.set_render_callback(render);
    
    while (1) {
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN) {
                if (game_state == STATE_TITLE || game_state == STATE_OVER || game_state == STATE_WIN) {
                    if (e.data1 & BTN_B) init_game();
                } else if (game_state == STATE_PLAY) {
                    if (e.data1 & BTN_UP)    player.next_dir = DIR_UP;
                    if (e.data1 & BTN_DOWN)  player.next_dir = DIR_DOWN;
                    if (e.data1 & BTN_LEFT)  player.next_dir = DIR_LEFT;
                    if (e.data1 & BTN_RIGHT) player.next_dir = DIR_RIGHT;
                }
            }
        }
        
        if (audio_timer > 0) {
            audio_timer--;
            if (audio_timer == 0) system_api.play_tone(0);
        }

        update();
        system_api.lcd_update();
        system_api.wait_tick();
    }
    
    return 0;
}