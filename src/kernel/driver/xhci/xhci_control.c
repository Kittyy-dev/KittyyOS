#include <stdint.h>
#include <string.h>

#include <xhci.h>
#include <vga.h>


struct usb_setup_packet {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));



static void xhci_ep0_push_trb(struct xhci_trb* src)
{
    ep0_ring[ep0_index] = *src;

    ep0_ring[ep0_index].d3 |= ep0_cycle;


    ep0_index++;

    if (ep0_index >= 64) {
        ep0_index = 0;
        ep0_cycle ^= 1;
    }


    uint32_t dboff = xhci_cap[5] & ~0x3;

    volatile uint32_t* doorbell =
        (volatile uint32_t*)(g_xhci_mmio + dboff);


    doorbell[g_slot_id] = 1;
}



int xhci_control_transfer(
    int port,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    void* buffer
)
{
    (void)port;


    ep0_index = 0;


    struct usb_setup_packet setup;

    setup.bmRequestType = request_type;
    setup.bRequest      = request;
    setup.wValue        = value;
    setup.wIndex        = index;
    setup.wLength       = length;


    struct xhci_trb trb;


    // Setup Stage

    memset(&trb, 0, sizeof(trb));

    memcpy(&trb.d0, &setup, sizeof(setup));

    trb.d2 = sizeof(setup);

    trb.d3 =
        (TRB_TYPE_SETUP_STAGE << 10) |
        TRB_IDT;


    xhci_ep0_push_trb(&trb);



    // Data Stage

    if (length > 0) {

        memset(&trb, 0, sizeof(trb));


        uint64_t buf =
            (uint64_t)(uintptr_t)buffer;


        trb.d0 = (uint32_t)buf;
        trb.d1 = (uint32_t)(buf >> 32);

        trb.d2 = length;


        trb.d3 =
            (TRB_TYPE_DATA_STAGE << 10) |
            (1 << 16);


        xhci_ep0_push_trb(&trb);
    }



    // Status Stage

    memset(&trb, 0, sizeof(trb));

    trb.d3 =
        (TRB_TYPE_STATUS_STAGE << 10);


    xhci_ep0_push_trb(&trb);



    return xhci_wait_for_event();
}




int xhci_get_device_descriptor(
    int port,
    void* buffer,
    int max_len
)
{
    return xhci_control_transfer(
        port,
        0x80,
        6,
        (1 << 8),
        0,
        max_len,
        buffer
    );
}




int xhci_get_config_descriptor(
    int port,
    void* buffer,
    int max_len
)
{
    return xhci_control_transfer(
        port,
        0x80,
        6,
        (2 << 8),
        0,
        max_len,
        buffer
    );
}