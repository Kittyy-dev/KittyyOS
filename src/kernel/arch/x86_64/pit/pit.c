#include <stdint.h>
#include <pit.h>
#include <ports.h>

#define PIT_COMMAND   0x43
#define PIT_CHANNEL0  0x40
#define PIT_FREQUENCY 1193182

volatile uint64_t timer_ticks = 0;

void pit_init(uint32_t frequency) {
    if (frequency == 0) {
        return;
    }

    uint32_t divisor = PIT_FREQUENCY / frequency;

    if (divisor < 1) {
        divisor = 1;
    }

    if (divisor > 65535) {
        divisor = 65535;
    }

    timer_ticks = 0;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

uint64_t timer_get_ticks(void) {
    return __atomic_load_n(&timer_ticks, __ATOMIC_RELAXED);
}

void pit_tick_handler(void) {
    __atomic_add_fetch(&timer_ticks, 1, __ATOMIC_RELAXED);
}