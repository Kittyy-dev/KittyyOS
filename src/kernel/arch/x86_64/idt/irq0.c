#include <stdint.h>
#include <ports.h>
#include <kernel_api.h>
#include <tsc.h>
#include <kprint.h>

extern volatile uint64_t timer_ticks;

void irq0_handler(void) {
    timer_ticks++;

    // kernel_schedule();

    if (timer_ticks % 10 == 0) {
        kernel_schedule();
    }

    // outb(0x20, 0x20);
}