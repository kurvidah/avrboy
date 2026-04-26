#include "timer.h"
#include "event.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint32_t system_ticks = 0;
static volatile uint8_t tick_flag = 0;

void timer_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, 64 prescaler
    OCR1A = (uint16_t)((F_CPU / 64 / 60) - 1);
    
    TIMSK1 |= (1 << OCIE1A); // Enable Compare Match A interrupt
}

uint32_t timer_get_ticks(void) {
    uint32_t ticks;
    uint8_t sreg = SREG;
    cli();
    ticks = system_ticks;
    SREG = sreg;
    return ticks;
}

void timer_wait_tick(void) {
    while (!tick_flag);
    tick_flag = 0;
}

ISR(TIMER1_COMPA_vect) {
    system_ticks++;
    tick_flag = 1;
    
    // Push a tick event to the queue
    event_push(EVENT_TICK, (uint8_t)(system_ticks & 0xFF), 0);
}
