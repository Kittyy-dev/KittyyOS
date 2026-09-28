#include <stdint.h>
#include <xhci.h>
#include <vga.h>
#include <ports.h>
#include <kprint.h>

static inline volatile uint32_t* xhci_get_ir0(void) {
    uint32_t rts = xhci_cap[6];
    volatile uint32_t* rt = (volatile uint32_t*)(g_xhci_mmio + (rts & ~0x1F));
    return (volatile uint32_t*)((uintptr_t)rt + 0x20); // IRS0
}

static inline void lapic_eoi(void) {
    *(volatile uint32_t*)0xFEE000B0 = 0;
}

void xhci_irq_handler(void) {
    volatile uint32_t *ir0 = xhci_get_ir0();

    while (1) {

        struct xhci_trb *ev = event_dequeue;

        if ((ev->d3 & 1) != event_cycle)
            break;

        uint32_t type = (ev->d3 >> 10) & 0x3F;

        kprintf("xHCI Event %u\n", type);

        switch (type) {
        case 32:    // Transfer Event
            break;

        case 33:    // Command Completion Event
            break;

        case 34:    // Port Status Change Event
            break;

        default:
            break;
        }

        event_dequeue++;

        if (event_dequeue >= &event_ring[64]) {
            event_dequeue = event_ring;
            event_cycle ^= 1;
        }
    }

    uint64_t ptr = (uint64_t)(uintptr_t)event_dequeue;

    ir0[6] = (uint32_t)(ptr | (1 << 3));   // EHB
    ir0[7] = (uint32_t)(ptr >> 32);

    ir0[0] |= (1 << 1);    // Clear Interrupt Pending

    lapic_eoi();
}