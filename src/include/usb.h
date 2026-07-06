#ifndef USB_H
#define USB_H

#include <stdint.h>

int usb_init();  
int usb_has_valid_fat32();
int usb_read_lba(uint32_t lba, uint8_t count, uint8_t* buffer);

#endif
