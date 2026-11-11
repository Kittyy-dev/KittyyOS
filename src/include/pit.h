#ifndef PIT_H
#define PIT_H

#include <stdint.h>

void pit_init(uint32_t frequency);
void pit_tick_handler(void);
uint64_t timer_get_ticks(void);

#endif