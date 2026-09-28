#include <stdint.h>

#include <xhci.h>
#include <vga.h>
#include <colors.h>
#include <types.h>
#include <kprint.h>

int xhci_wait_for_event(void) {
    uint32_t rts_offset = *(volatile uint32_t *)(g_xhci_mmio + 0x18);

    volatile uint32_t *xhci_rt =
        (volatile uint32_t *)(g_xhci_mmio + rts_offset);

    volatile uint32_t *ir0 =
        (volatile uint32_t *)((uintptr_t)xhci_rt + 0x20);

    uint64_t erdp =
        ((uint64_t)ir0[7] << 32) | ir0[6];

    struct xhci_trb *ev =
        (struct xhci_trb *)(erdp & ~0xF);

    for (uint32_t i = 0; i < 100000000; i++)
    {
        uint32_t type  = (ev->d3 >> 10) & 0x3F;
        uint32_t cycle = ev->d3 & 1;

        if (type != 0 && cycle == event_cycle)
        {
            g_last_event = ev;
            return 0;
        }
    }

    g_last_event = NULL;

    kprintf(WHITE "<" RED " ERROR " WHITE "> Event timeout\n");

    return -1;
}