#ifndef USB_H
#define USB_H

#include <stdint.h>

struct usb_descriptor_header {
    uint8_t bLength;
    uint8_t bDescriptorType;
} __attribute__((packed));


struct usb_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed));


struct usb_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed));

struct usb_setup_packet {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));

struct scsi_read10_cdb {
    uint8_t  opcode;      // 0x28
    uint8_t  flags;
    uint32_t lba;         // Big Endian
    uint8_t  reserved;
    uint16_t transfer_len;// Big Endian
    uint8_t  control;
} __attribute__((packed));

struct usb_msc_cbw {
    uint32_t signature;      // 'USBC' = 0x43425355
    uint32_t tag;
    uint32_t data_transfer_len;
    uint8_t  flags;          // 0x80 = IN
    uint8_t  lun;
    uint8_t  cb_length;
    uint8_t  cdb[16];
} __attribute__((packed));

struct usb_msc_csw {
    uint32_t signature;      // 'USBS' = 0x53425355
    uint32_t tag;
    uint32_t data_residue;
    uint8_t  status;         // 0 = Passed
} __attribute__((packed));

int usb_init();  
int usb_has_valid_fat32();
int usb_read_lba(uint32_t lba, uint8_t count, uint8_t* buffer);

#endif
