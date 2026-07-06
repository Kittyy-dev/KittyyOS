#include <stdint.h>
#include <ports.h>

uint64_t tsc_freq = 0;

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void calibrate_tsc() {
    // PIT Channel 0, mode 2, reload 0xFFFF
    outb(0x43, 0x34);
    outb(0x40, 0xFF);
    outb(0x40, 0xFF);

    uint64_t start = rdtsc();

    // ersten Wert holen
    outb(0x43, 0x00);
    uint8_t lo0 = inb(0x40);
    uint8_t hi0 = inb(0x40);
    uint16_t prev = (hi0 << 8) | lo0;

    // warten bis der Counter einmal wrappt
    while (1) {
        outb(0x43, 0x00);
        uint8_t lo = inb(0x40);
        uint8_t hi = inb(0x40);
        uint16_t val = (hi << 8) | lo;

        // Wrap erkannt: neuer Wert > vorheriger
        if (val > prev)
            break;

        prev = val;
    }

    uint64_t end = rdtsc();
    uint64_t diff = end - start;

    // 65536 PIT‑Ticks ≈ 54.9254 ms → CPU Hz ≈ diff / 0.0549
    tsc_freq = (uint64_t)((double)diff * 18.2065);
}

void sleep_ms(uint32_t ms) {
    uint64_t ticks = (tsc_freq / 1000) * ms;
    uint64_t start = rdtsc();
    uint64_t end = start + ticks;

    while (rdtsc() < end) {
        __asm__ volatile("pause");
    }
}

void sleep_us(uint32_t us) {
    uint64_t ticks = (tsc_freq / 1000000) * us;
    uint64_t start = rdtsc();
    uint64_t end = start + ticks;

    while (rdtsc() < end) {
        __asm__ volatile("pause");
    }
}
