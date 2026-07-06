#include <stdint.h>
#include <xhci.h>
#include <vga.h>
#include <ports.h>

static inline volatile uint32_t* xhci_get_ir0(void) {
    uint32_t rts = xhci_cap[2];
    volatile uint32_t* rt = (volatile uint32_t*)(g_xhci_mmio + (rts & ~0x1F));
    return (volatile uint32_t*)((uintptr_t)rt + 0x20); // IRS0
}

void xhci_irq_handler(void) {
    volatile uint32_t* ir0 = xhci_get_ir0();

    uint64_t erdp = ((uint64_t)ir0[7] << 32) | ir0[6];
    struct xhci_trb* ev = (struct xhci_trb*)(erdp & ~0xF);

    uint32_t type  = (ev->d3 >> 10) & 0x3F;
    uint32_t cycle = ev->d3 & 1;

    if (cycle == event_cycle) {
        vga_printf("xHCI IRQ: type=%u d0=%x d1=%x d2=%x d3=%x\n",
                   type, ev->d0, ev->d1, ev->d2, ev->d3);

        erdp += sizeof(struct xhci_trb);
        ir0[6] = (uint32_t)erdp | event_cycle;
        ir0[7] = (uint32_t)(erdp >> 32);

        ir0[0] |= (1 << 1);
    }

    *(volatile uint32_t*)0xFEE000B0 = 0;
}