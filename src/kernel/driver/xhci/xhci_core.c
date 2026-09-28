#include <stdint.h>
#include <string.h>

#include <xhci.h>
#include <pci.h>
#include <vga.h>
#include <colors.h>
#include <paging.h>
#include <kprint.h>


#define USBCMD_RUN   (1 << 0)
#define USBCMD_HCRST (1 << 1)
#define USBSTS_CNR   (1 << 11)

#define PORTSC_CCS   (1 << 0)
#define PORTSC_PED   (1 << 1)
#define PORTSC_PR    (1 << 4)
#define PORTSC_PP    (1 << 9)
#define USBCMD_INTE  (1 << 2)

static void xhci_reset() {
    xhci_op[0] |= USBCMD_HCRST;
    while (xhci_op[0] & USBCMD_HCRST);
    while (xhci_op[1] & USBSTS_CNR);
}

static void xhci_start() {
    xhci_op[0] |= USBCMD_RUN | USBCMD_INTE;
}

void xhci_enable_ports() {
    uint32_t hcsparams1 = xhci_cap[1];
    uint32_t num_ports = hcsparams1 & 0xFF;

    volatile uint32_t* ports = xhci_op + (0x400 / 4);

    for (uint32_t i = 0; i < num_ports; i++) {
        ports[i * 4] |= PORTSC_PP;
    }
}

int xhci_find_device_port() {
    uint32_t hcsparams1 = xhci_cap[1];
    uint32_t num_ports = hcsparams1 & 0xFF;

    volatile uint32_t* ports = xhci_op + (0x400 / 4);

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> Scanning XHCI ports...\n");
    for (uint32_t i = 0; i < num_ports; i++) {
        uint32_t portsc = ports[i * 4];
    }

    for (uint32_t i = 0; i < num_ports; i++) {

        if (!(ports[i * 4] & PORTSC_CCS))
            continue;

        ports[i * 4] |= PORTSC_PR;

        while (ports[i * 4] & PORTSC_PR) {}

        if (ports[i * 4] & PORTSC_PED)
            return i;
    }

    return -1;
}

int xhci_init(void) {
    uint8_t header = pci_header_type();
    int is_multi = header & 0x80;

    pci_scan_bus(0);

    if (!is_multi) {
        if (!pci_xhci_found) return -1;
    } else {
        for (int func = 1; func < 8; func++) {
            uint16_t vendor = pci_read_word(0, 0, func, 0x00);
            if (vendor != 0xFFFF) {
                uint8_t secondary_bus = pci_read_byte(0, 0, func, 0x19);
                pci_scan_bus(secondary_bus);
            }
        }
        if (!pci_xhci_found) return -1;
    }

    uint32_t bar0 = pci_read_dword(pci_xhci_bus, pci_xhci_slot, pci_xhci_func, 0x10);
    uint32_t bar1 = pci_read_dword(pci_xhci_bus, pci_xhci_slot, pci_xhci_func, 0x14);

    if ((bar0 & 0x6) == 0x4) {
        g_xhci_mmio = ((uint64_t)bar1 << 32) | (bar0 & ~0xF);
    } else {
        g_xhci_mmio = bar0 & ~0xF;
    }

    // g_xhci_mmio = (uintptr_t)(bar0 & ~0xF);

    // mapping
    for (uint64_t addr = g_xhci_mmio; addr < g_xhci_mmio + 0x10000; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    }

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "XHCI Mapped!\n");

    xhci_cap = (volatile uint32_t*)g_xhci_mmio;

    uint8_t cap_length = *((volatile uint8_t*)xhci_cap);
    xhci_op = (volatile uint32_t*)(g_xhci_mmio + cap_length);

    uint32_t rts_offset = *(volatile uint32_t*)(g_xhci_mmio + 0x18);   
    volatile uint8_t* xhci_rt_base = (volatile uint8_t*)(g_xhci_mmio + rts_offset);

    xhci_rt = (volatile uint32_t*)xhci_rt_base;


    xhci_reset();
    volatile uint32_t *config = xhci_op + (0x38 / 4);
    config[0] = 1;

    xhci_start();

    memset(cmd_ring, 0, sizeof(cmd_ring));
    cmd_cycle = 1;
    cmd_ring_index = 0;

    // Command Ring Pointer setzen
    volatile uint32_t* xhci_doorbell = (volatile uint32_t*)(g_xhci_mmio + 0x20); // DB-Register-Basis
    volatile uint32_t* xhci_crcr = (volatile uint32_t*)((uintptr_t)xhci_op + 0x18);

    uintptr_t cr_ptr = (uintptr_t)&cmd_ring[0];

    xhci_crcr[0] = (uint32_t)cr_ptr | cmd_cycle;
    xhci_crcr[1] = (uint32_t)(cr_ptr >> 32);

    memset(&g_dev_ctx, 0, sizeof(g_dev_ctx));
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));
    memset(g_dcbaa, 0, sizeof(g_dcbaa));

    // DCBAA[slot_id] zeigt auf Device Context
    // g_dcbaa[g_slot_id] = (uint64_t)(uintptr_t)&g_dev_ctx;

    // DCBAA in die Operational Registers schreiben (DCBAAP)
    volatile uint32_t* dcbaap = (volatile uint32_t*)((uintptr_t)xhci_op + 0x30);
    uint64_t dcbaap_ptr = (uint64_t)(uintptr_t)g_dcbaa;

    dcbaap[0] = (uint32_t)dcbaap_ptr;
    dcbaap[1] = (uint32_t)(dcbaap_ptr >> 32);

    memset(ep0_ring, 0, sizeof(ep0_ring));
    ep0_cycle = 1;

    ep0_ring[63].d0 = (uint32_t)(uintptr_t)ep0_ring;
    ep0_ring[63].d1 = (uint32_t)((uint64_t)ep0_ring >> 32);
    ep0_ring[63].d2 = 0;
    ep0_ring[63].d3 = (TRB_TYPE_LINK << 10) | (1 << 1) | ep0_cycle;

    // bulk out
    memset(bulk_out_ring, 0, sizeof(bulk_out_ring));
    bulk_out_cycle = 1;
    bulk_out_index = 0;

    bulk_out_ring[63].d0 = (uint32_t)(uintptr_t)bulk_out_ring;
    bulk_out_ring[63].d1 = (uint32_t)((uint64_t)bulk_out_ring >> 32);
    bulk_out_ring[63].d2 = 0;
    bulk_out_ring[63].d3 = (TRB_TYPE_LINK << 10) | (1 << 1) | bulk_out_cycle;

    // bulk in
    memset(bulk_in_ring, 0, sizeof(bulk_in_ring));
    bulk_in_cycle = 1;
    bulk_in_index = 0;

    bulk_in_ring[63].d0 = (uint32_t)(uintptr_t)bulk_in_ring;
    bulk_in_ring[63].d1 = (uint32_t)((uint64_t)bulk_in_ring >> 32);
    bulk_in_ring[63].d2 = 0;
    bulk_in_ring[63].d3 = (TRB_TYPE_LINK << 10) | (1 << 1) | bulk_in_cycle;

    // event ring

    memset(event_ring, 0, sizeof(event_ring));
    event_cycle = 1;
    event_dequeue = event_ring;

    event_ring[63].d0 = (uint32_t)(uintptr_t)event_ring;
    event_ring[63].d1 = (uint32_t)((uint64_t)event_ring >> 32);
    event_ring[63].d2 = 0;
    event_ring[63].d3 = (TRB_TYPE_LINK << 10) | (1 << 1) | event_cycle;


    memset(&g_erst, 0, sizeof(g_erst));
    g_erst.base = (uint64_t)(uintptr_t)event_ring;
    g_erst.size = 64; // 64 TRBs
    g_erst.rsvd = 0;

    uintptr_t rt = g_xhci_mmio + (xhci_cap[6] & ~0x1F);
    uintptr_t ir = rt + 0x20;

    /*
    for (int i = 0; i < 8; i++) {
        kprintf("[%d] = %x\n", i, xhci_cap[i]);
    } */

    volatile uint32_t* ir0 = (volatile uint32_t*)ir;

    // volatile uint32_t* ir0 = (volatile uint32_t*)((uintptr_t)xhci_rt + 0x20);

    // ERSTSZ
    ir0[2] = 1; 

    // ERSTBA (low/high)
    uint64_t erst_ptr = (uint64_t)(uintptr_t)&g_erst;
    ir0[4] = (uint32_t)erst_ptr;
    ir0[5] = (uint32_t)(erst_ptr >> 32);

    uint64_t erdp = (uint64_t)(uintptr_t)event_ring;
    ir0[6] = (uint32_t)erdp;
    ir0[7] = (uint32_t)(erdp >> 32);

    ir0[1] |= 1;
    ir0[0] |= 2;

    int slot_id = xhci_enable_slot();

    if (slot_id < 0) {
        return -1;
    }

    g_slot_id = slot_id;

    g_dcbaa[g_slot_id] = (uint64_t)(uintptr_t)&g_dev_ctx;

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> XHCI Slot enabled: %d\n", slot_id);

    if (xhci_wait_for_event() < 0) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Enable Slot failed\n");
    }

    if (xhci_wait_for_event() < 0) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Address Device failed\n");
    }

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> Device addressed successfully\n");

    /*
    kprintf("event_ring[0]: d0=%x d1=%x d2=%x d3=%x\n",
           event_ring[0].d0,
           event_ring[0].d1,
           event_ring[0].d2,
           event_ring[0].d3);
    */

    // kprintf(WHITE "<" GREEN " KERNEL " WHITE "> XHCI device addressed.\n");

    return 0;
}