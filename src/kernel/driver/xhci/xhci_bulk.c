#include <stdint.h>
#include <string.h>

#include <xhci.h>
#include <vga.h>


struct xhci_trb bulk_out_ring[64] __attribute__((aligned(64)));
struct xhci_trb bulk_in_ring[64]  __attribute__((aligned(64)));


uint32_t bulk_out_index = 0;
uint32_t bulk_in_index  = 0;

uint8_t bulk_out_cycle = 1;
uint8_t bulk_in_cycle  = 1;


uint8_t g_msc_bulk_out_ep = 0x01;
uint8_t g_msc_bulk_in_ep  = 0x81;



void xhci_bulk_out_push_trb(struct xhci_trb* trb)
{
    bulk_out_ring[bulk_out_index] = *trb;

    bulk_out_ring[bulk_out_index].d3 |= bulk_out_cycle;


    bulk_out_index++;

    if (bulk_out_index >= 63) {
        bulk_out_index = 0;
        bulk_out_cycle ^= 1;
    }


    uint32_t dboff = xhci_cap[5] & ~0x3;

    volatile uint32_t* doorbell =
        (volatile uint32_t*)(g_xhci_mmio + dboff);


    doorbell[g_slot_id] = 2;
}



int xhci_bulk_out(uint8_t ep, const void* data, uint32_t len)
{
    (void)ep;


    struct xhci_trb trb;

    memset(&trb, 0, sizeof(trb));


    uint64_t ptr = (uint64_t)(uintptr_t)data;


    trb.d0 = (uint32_t)ptr;
    trb.d1 = (uint32_t)(ptr >> 32);
    trb.d2 = len;

    trb.d3 =
        (TRB_TYPE_TRANSFER << 10) |
        TRB_IOC;


    xhci_bulk_out_push_trb(&trb);


    return xhci_wait_for_event();
}



int xhci_bulk_in(uint8_t ep, void* buffer, uint32_t len) {
    (void)ep;
    (void)buffer;
    (void)len;


    return -1;
}