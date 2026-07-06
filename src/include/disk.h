#ifndef DISK_H
#define DISK_H

#include <stdint.h>

extern uint32_t g_part_lba_start;

void read_sectors_abs(uint32_t lba, uint8_t* buffer, uint8_t count);
void read_sectors(uint32_t rel_lba, uint8_t* buffer);
void write_sectors(uint32_t rel_lba, const uint8_t* buffer);

#endif