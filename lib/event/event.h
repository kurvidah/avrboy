#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_BTN_DOWN,
    EVENT_BTN_UP,
    EVENT_JOY_MOVE,
    EVENT_TICK,
    EVENT_CART_INSERT,
} event_type_t;

typedef struct {
    uint8_t type;
    uint8_t data1;
    uint8_t data2;
} Event;

#define BTN_UP    (1 << 0)
#define BTN_DOWN  (1 << 1)
#define BTN_LEFT  (1 << 2)
#define BTN_RIGHT (1 << 3)
#define BTN_A     (1 << 4)
#define BTN_B     (1 << 5)

void event_init(void);
bool event_push(event_type_t type, uint8_t data1, uint8_t data2);
bool event_poll(Event* e);

