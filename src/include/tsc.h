#include <stdint.h>

static inline uint64_t rdtsc();

void calibrate_tsc();

void sleep_ms(uint32_t ms);

void sleep_us(uint32_t us);