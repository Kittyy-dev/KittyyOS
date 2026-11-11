#include <stdint.h>

uint64_t rdtsc();

void calibrate_tsc();

void sleep_ms(uint32_t ms);

void sleep_us(uint32_t us);

bool tsc_timeout(uint32_t timeout_ms, uint64_t start_tsc);
uint64_t get_tsc_freq();