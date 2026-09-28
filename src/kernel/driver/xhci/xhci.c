#include <string.h>
#include <pci.h>
#include <xhci.h>
#include <vga.h>
#include <colors.h>
#include <paging.h>
#include <usb.h>
#include <kprint.h>

#define USBCMD_RUN   (1 << 0)
#define USBCMD_HCRST (1 << 1)
#define USBSTS_CNR   (1 << 11)

#define PORTSC_CCS   (1 << 0)
#define PORTSC_PED   (1 << 1)
#define PORTSC_PR    (1 << 4)
#define PORTSC_PP    (1 << 9)
#define USBCMD_INTE  (1 << 2)

int xhci_enable_slot(void)
{
    struct xhci_trb trb;

    memset(&trb, 0, sizeof(trb));

    trb.d3 = (9 << 10);

    // hier???

    xhci_push_cmd_trb(&trb);

    if (xhci_wait_for_event() < 0)
        return -1;

    if (!g_last_event)
        return -1;

    uint8_t slot = (g_last_event->d3 >> 24) & 0xFF;

    // kprintf("Enable Slot Event: type %d slot %d code %d\n", (g_last_event->d3 >> 10) & 0x3F, slot, (g_last_event->d2 >> 24) & 0xFF);

    return (g_last_event->d3 >> 24) & 0xFF;
}

/*
int xhci_address_device() {
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));

    g_input_ctx.ctrl.add_flags = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    g_input_ctx.slot.d[0] = (3 << 27);
    g_input_ctx.slot.d[1] = 0; 

    uint64_t ep0_ring_ptr = (uint64_t)(uintptr_t)ep0_ring;

    g_input_ctx.ep0.d[0] = (4 << 10);          // Endpoint Type = Control (4)
    g_input_ctx.ep0.d[1] = (64 << 16);        
    g_input_ctx.ep0.d[2] = (uint32_t)ep0_ring_ptr;
    g_input_ctx.ep0.d[3] = (uint32_t)(ep0_ring_ptr >> 32);

    struct xhci_trb trb;
    memset(&trb, 0, sizeof(trb));

    uint64_t ictx_ptr = (uint64_t)(uintptr_t)&g_input_ctx;

    trb.d0 = (uint32_t)ictx_ptr;
    trb.d1 = (uint32_t)(ictx_ptr >> 32);
    trb.d2 = g_slot_id;             
    trb.d3 = (11 << 10);            

    xhci_push_cmd_trb(&trb);

    return 0;
}
*/

int xhci_address_device() {
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));

    g_input_ctx.ctrl.drop_flags = 0;
    g_input_ctx.ctrl.add_flags = (1 << 0) | (1 << 1);

    g_input_ctx.slot.d[0] = (1 << 27);

    uint64_t ring = (uint64_t)(uintptr_t)ep0_ring;

    g_input_ctx.ep0.d[0] = (4 << 3);
    g_input_ctx.ep0.d[1] = (64 << 16);
    g_input_ctx.ep0.d[2] = (uint32_t)ring;
    g_input_ctx.ep0.d[3] = (uint32_t)(ring >> 32);

    struct xhci_trb trb;
    memset(&trb, 0, sizeof(trb));

    uint64_t input_ctx = (uint64_t)(uintptr_t)&g_input_ctx;

    trb.d0 = (uint32_t)input_ctx;
    trb.d1 = (uint32_t)(input_ctx >> 32);

    trb.d3 = (11 << 10) |
             (g_slot_id << 24) |
             cmd_cycle;

    xhci_push_cmd_trb(&trb);

    return xhci_wait_for_event();
}

int xhci_configure_endpoints(void) { 
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));

    // slot context + ep0 out + ep1 in 
    g_input_ctx.ctrl.add_flags = (1 << 0) | (1 << 2) | (1 << 3);

    g_input_ctx.slot.d[0] = (3 << 27);

    struct xhci_trb trb;
    memset(&trb, 0, sizeof(trb));

    uint64_t ictx = (uint64_t)(uintptr_t)&g_input_ctx;

    trb.d0 = (uint32_t)ictx;
    trb.d1 = (uint32_t)(ictx >> 32);
    trb.d3 = (12 << 10);

    xhci_push_cmd_trb(&trb);

    if (xhci_wait_for_event() < 0) {
        return -1;
    }

    return 0;
}

static uint32_t be32(uint32_t v) {
    return ((v & 0x000000FF) << 24) |
           ((v & 0x0000FF00) << 8)  |
           ((v & 0x00FF0000) >> 8)  |
           ((v & 0xFF000000) >> 24);
}

static uint16_t be16(uint16_t v) {
    return (uint16_t)(((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8));
}