#include "event.h"
#include <avr/interrupt.h>

#define EVENT_QUEUE_SIZE 8

static Event event_queue[EVENT_QUEUE_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;

void event_init(void) {
    head = 0;
    tail = 0;
}

bool event_push(event_type_t type, uint8_t data1, uint8_t data2) {
    uint8_t sreg = SREG;
    cli();

    uint8_t next_head = (head + 1) % EVENT_QUEUE_SIZE;

    // Check for overflow (Drop new event if full)
    if (next_head == tail) {
        SREG = sreg;
        return false;
    }

    event_queue[head].type = type;
    event_queue[head].data1 = data1;
    event_queue[head].data2 = data2;
    
    head = next_head;

    SREG = sreg;
    return true;
}

bool event_poll(Event* e) {
    // If head == tail, queue is empty
    if (head == tail) {
        return false;
    }

    // Single consumer: head is only modified by producer, tail only by consumer
    e->type = event_queue[tail].type;
    e->data1 = event_queue[tail].data1;
    e->data2 = event_queue[tail].data2;

    tail = (tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}
