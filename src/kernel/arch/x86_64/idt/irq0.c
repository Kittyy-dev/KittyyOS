#include <pit.h>
#include <scheduler.h>
#include <stdint.h>
#include <ports.h>
#include <kprint.h>

extern volatile uint64_t timer_ticks;

void irq0_handler(void) {
    pit_tick_handler();
    scheduler_tick();
}