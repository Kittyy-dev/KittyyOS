#include <stdint.h>
#include <sound.h>
#include <ports.h>

void pc_speaker_beep(uint32_t freq) {
    uint32_t div = 1193180 / freq;

    outb(0x43, 0xB6);
    outb(0x40, div & 0xFF);
    outb(0x40, div >> 8);

    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 3);
}

void pc_speaker_stop() {
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & ~3);
}