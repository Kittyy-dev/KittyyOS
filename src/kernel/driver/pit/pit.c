#include <stdint.h>
#include <pit.h>
#include <ports.h>

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

#define PIT_FREQUENCY 1193182

volatile uint64_t timer_ticks = 0;

void pit_init(uint32_t frequency) {
    uint32_t divisor = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, divisor & 0xFF);

    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

uint64_t timer_get_ticks(void) {
    return timer_ticks;
}