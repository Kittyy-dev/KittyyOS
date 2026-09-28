#include <stdint.h>
#include <string.h>

#include <xhci.h>
#include <usb.h>


static uint32_t be32(uint32_t v)
{
    return ((v & 0x000000FF) << 24) |
           ((v & 0x0000FF00) << 8)  |
           ((v & 0x00FF0000) >> 8)  |
           ((v & 0xFF000000) >> 24);
}


static uint16_t be16(uint16_t v)
{
    return (uint16_t)(((v & 0x00FF) << 8) |
                      ((v & 0xFF00) >> 8));
}


/* ---------- SCSI READ10 ---------- */

struct scsi_read10_cdb {
    uint8_t  opcode;
    uint8_t  flags;
    uint32_t lba;
    uint8_t  reserved;
    uint16_t transfer_len;
    uint8_t  control;
} __attribute__((packed));


struct usb_msc_cbw {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_transfer_len;

    uint8_t flags;
    uint8_t lun;
    uint8_t cb_length;

    uint8_t cdb[16];

} __attribute__((packed));


struct usb_msc_csw {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_residue;

    uint8_t status;

} __attribute__((packed));



int xhci_scsi_read10(uint32_t lba, uint8_t count, uint8_t* buffer) {

    struct scsi_read10_cdb cdb;
    cdb.opcode       = 0x28;
    cdb.flags        = 0x00;
    cdb.lba          = be32(lba);
    cdb.reserved     = 0;
    cdb.transfer_len = be16(count);
    cdb.control      = 0;

    struct usb_msc_cbw cbw;
    memset(&cbw, 0, sizeof(cbw));
    cbw.signature         = 0x43425355; // 'USBC'
    cbw.tag               = 0x12345678;
    cbw.data_transfer_len = count * 512;
    cbw.flags             = 0x80;       // IN
    cbw.lun               = 0;
    cbw.cb_length         = 10;

    memcpy(cbw.cdb, &cdb, sizeof(cdb));

    if (xhci_bulk_out(g_msc_bulk_out_ep, &cbw, sizeof(cbw)) < 0)
        return -1;

    if (xhci_bulk_in(g_msc_bulk_in_ep, buffer, count * 512) < 0)
        return -1;

    struct usb_msc_csw csw;
    if (xhci_bulk_in(g_msc_bulk_in_ep, &csw, sizeof(csw)) < 0)
        return -1;

    if (csw.signature != 0x53425355)
        return -1;

    if (csw.status != 0)
        return -1;

    return 0;
}

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

    uint8_t bLength;
    uint8_t bDescriptorType;

    uint8_t bEndpointAddress;

    uint8_t bmAttributes;

    uint16_t wMaxPacketSize;

    uint8_t bInterval;

} __attribute__((packed));

int xhci_init_mass_storage(int port) {

    uint8_t buf[512];
    if (xhci_get_config_descriptor(port, buf, sizeof(buf)) < 0)
        return -1;

    uint16_t total_len = buf[2] | (buf[3] << 8);
    if (total_len > sizeof(buf))
        total_len = sizeof(buf);

    uint16_t offset = buf[0]; // erste bLength (Config selbst)
    uint8_t  found_interface = 0;

    while (offset + 2 <= total_len) {
        struct usb_descriptor_header* hdr =
            (struct usb_descriptor_header*)(buf + offset);

        if (hdr->bLength == 0)
            break;

        if (hdr->bDescriptorType == 4) { // Interface
            struct usb_interface_descriptor* ifd =
                (struct usb_interface_descriptor*)hdr;

            if (ifd->bInterfaceClass == 0x08) { // Mass Storage
                found_interface = 1;
            } else {
                found_interface = 0;
            }
        } else if (hdr->bDescriptorType == 5 && found_interface) { // Endpoint
            struct usb_endpoint_descriptor* ep =
                (struct usb_endpoint_descriptor*)hdr;

            if ((ep->bmAttributes & 0x03) == 2) { // Bulk
                if (ep->bEndpointAddress & 0x80)
                    g_msc_bulk_in_ep = ep->bEndpointAddress;
                else
                    g_msc_bulk_out_ep = ep->bEndpointAddress;
            }
        }

        offset += hdr->bLength;
    }

    if (g_msc_bulk_in_ep == 0 || g_msc_bulk_out_ep == 0)
        return -1;

    return 0;
}