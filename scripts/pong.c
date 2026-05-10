#include "avrboy.h"
#ifdef SIMULATOR
#include <stdlib.h>
#endif

// Screen dimensions
#define SCREEN_W 128
#define SCREEN_H 64

// Paddle dimensions
#define PADDLE_W 3
#define PADDLE_H 16
#define PADDLE_SPEED 3

// Ball dimensions
#define BALL_SIZE 2

// Initial positions
#define PADDLE_LEFT_X 4
#define PADDLE_RIGHT_X (SCREEN_W - PADDLE_W - 4)

// Game state
static int16_t ball_x = SCREEN_W / 2, ball_y = SCREEN_H / 2;
static int8_t ball_dx = 1, ball_dy = 0; // Serve straight and slow

static int16_t paddle_left_y = (SCREEN_H - PADDLE_H) / 2;
static int16_t paddle_right_y = (SCREEN_H - PADDLE_H) / 2;

static uint8_t score_left = 0;
static uint8_t score_right = 0;
static char score_str[10];

static uint8_t buttons_state = 0;
static uint8_t joy_y = 128; // Track joystick Y for W/S keys

void reset_ball() {
    ball_x = SCREEN_W / 2;
    ball_y = SCREEN_H / 2;
    // Reset to slow speed, sending to the opposite side
    ball_dx = (ball_dx > 0) ? -1 : 1;
    ball_dy = 0; // Serve straight
}

void int_to_str(uint8_t num, char* str) {
    if (num > 9) {
        str[0] = '0' + (num / 10);
        str[1] = '0' + (num % 10);
        str[2] = '\0';
    } else {
        str[0] = '0' + num;
        str[1] = '\0';
    }
}

void render(void) {
    // Draw center line
    for (uint8_t i = 0; i < SCREEN_H; i += 4) {
        system_api.draw_line(SCREEN_W / 2, i, SCREEN_W / 2, i + 2, 1);
    }
    
    // Draw scores
    int_to_str(score_left, score_str);
    system_api.draw_string((SCREEN_W / 4) - 4, 2, score_str, 1);
    
    int_to_str(score_right, score_str);
    system_api.draw_string((3 * SCREEN_W / 4) - 4, 2, score_str, 1);
    
    // Draw paddles
    system_api.fill_rect(PADDLE_LEFT_X, paddle_left_y, PADDLE_W, PADDLE_H, 1);
    system_api.fill_rect(PADDLE_RIGHT_X, paddle_right_y, PADDLE_W, PADDLE_H, 1);
    
    // Draw the ball
    system_api.fill_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 1);
}

APP_MAIN() {
    system_api.log("Pong Start");
    system_api.set_render_callback(render);
    
    while (1) {
        int8_t left_dir = 0;
        int8_t right_dir = 0;
        
        // Handle input events
        Event e;
        while (system_api.poll_event(&e)) {
            if (e.type == EVENT_BTN_DOWN || e.type == EVENT_BTN_UP) {
                buttons_state = e.data1;
            } else if (e.type == EVENT_JOY_MOVE) {
                joy_y = e.data2;
            }
        }
        
        // Update Right Paddle (Arrow keys / D-pad)
        if (buttons_state & BTN_UP) {
            paddle_right_y -= PADDLE_SPEED;
            right_dir = -1;
        }
        if (buttons_state & BTN_DOWN) {
            paddle_right_y += PADDLE_SPEED;
            right_dir = 1;
        }
        
        // Update Left Paddle (Z/X / A/B buttons OR W/S Joystick)
        if ((buttons_state & BTN_B) || joy_y < 64) { // Z or W
            paddle_left_y -= PADDLE_SPEED;
            left_dir = -1;
        }
        if ((buttons_state & BTN_A) || joy_y > 192) { // X or S
            paddle_left_y += PADDLE_SPEED;
            left_dir = 1;
        }

        // Constrain paddles
        if (paddle_left_y < 0) paddle_left_y = 0;
        if (paddle_left_y > SCREEN_H - PADDLE_H) paddle_left_y = SCREEN_H - PADDLE_H;
        
        if (paddle_right_y < 0) paddle_right_y = 0;
        if (paddle_right_y > SCREEN_H - PADDLE_H) paddle_right_y = SCREEN_H - PADDLE_H;
        
        // Move the ball
        ball_x += ball_dx;
        ball_y += ball_dy;
        
        // Bounce off top and bottom walls
        if (ball_y <= 0 || ball_y >= SCREEN_H - BALL_SIZE) {
            ball_dy = -ball_dy;
        }
        
        // Paddle collisions
        // Left paddle
        if (ball_x <= PADDLE_LEFT_X + PADDLE_W && ball_x >= PADDLE_LEFT_X && 
            ball_y + BALL_SIZE >= paddle_left_y && ball_y <= paddle_left_y + PADDLE_H) {
            if (left_dir != 0) {
                ball_dx = 2; // Faster if hit while moving
                ball_dy = left_dir; // Impart vertical direction
            } else {
                ball_dx = 1;
            }
            ball_x = PADDLE_LEFT_X + PADDLE_W; // Prevent sticking
        }
        
        // Right paddle
        if (ball_x + BALL_SIZE >= PADDLE_RIGHT_X && ball_x + BALL_SIZE <= PADDLE_RIGHT_X + PADDLE_W && 
            ball_y + BALL_SIZE >= paddle_right_y && ball_y <= paddle_right_y + PADDLE_H) {
            if (right_dir != 0) {
                ball_dx = -2; // Faster if hit while moving
                ball_dy = right_dir; // Impart vertical direction
            } else {
                ball_dx = -1;
            }
            ball_x = PADDLE_RIGHT_X - BALL_SIZE; // Prevent sticking
        }
        
        // Scoring
        if (ball_x < 0) {
            score_right++;
            reset_ball();
        } else if (ball_x > SCREEN_W) {
            score_left++;
            reset_ball();
        }
        
        // Exit condition (Left + Right together, or just Left since it's unused)
        if (buttons_state & BTN_LEFT) { 
#ifdef SIMULATOR
             exit(0);
#else
             ((void (*)(void))0x0000)(); // Reset to OS
#endif
        }

        system_api.lcd_update();
        system_api.wait_tick();
    }
    
    return 0;
}
