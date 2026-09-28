#include <stdint.h>

#include <xhci.h>


void xhci_push_cmd_trb(struct xhci_trb* trb)
{
    cmd_ring[cmd_ring_index] = *trb;

    cmd_ring[cmd_ring_index].d3 |= cmd_cycle;


    cmd_ring_index++;

    if (cmd_ring_index >= 64)
    {
        cmd_ring_index = 0;
        cmd_cycle ^= 1;
    }


    uint32_t dboff = xhci_cap[5] & ~0x3;


    volatile uint32_t* doorbell =
        (volatile uint32_t*)(g_xhci_mmio + dboff);


    // Doorbell 0 = Command Ring
    doorbell[0] = 0;
}



void xhci_ep0_push_trb(struct xhci_trb* trb)
{
    ep0_ring[ep0_index] = *trb;

    ep0_ring[ep0_index].d3 |= ep0_cycle;


    ep0_index++;

    if (ep0_index >= 64)
    {
        ep0_index = 0;
        ep0_cycle ^= 1;
    }


    uint32_t dboff = xhci_cap[5] & ~0x3;


    volatile uint32_t* doorbell =
        (volatile uint32_t*)(g_xhci_mmio + dboff);


    doorbell[g_slot_id] = 1;
}