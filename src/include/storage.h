#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

extern int g_use_usb;
extern int g_use_ahci;
extern int g_use_ata;

void storage_init();
int mbr_matches_boot_disk(const uint8_t* mbr);

#endif
