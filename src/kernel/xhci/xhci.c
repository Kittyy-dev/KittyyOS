#include <stdint.h>
#include <string.h>
#include <pci.h>
#include <xhci.h>
#include <vga.h>
#include <colors.h>
#include <paging.h>
#include <usb.h>

#define TRB_TYPE_SETUP_STAGE   2
#define TRB_TYPE_DATA_STAGE    3
#define TRB_TYPE_STATUS_STAGE  4
#define TRB_TYPE_EVENT_DATA    5
#define TRB_TYPE_TRANSFER      32
#define TRB_TYPE_LINK          6

#define TRB_CYCLE  (1 << 0)
#define TRB_IOC    (1 << 5)
#define TRB_IDT    (1 << 6)

#define USBCMD_RUN   (1 << 0)
#define USBCMD_HCRST (1 << 1)
#define USBSTS_CNR   (1 << 11)

#define PORTSC_CCS   (1 << 0)
#define PORTSC_PED   (1 << 1)
#define PORTSC_PR    (1 << 4)
#define PORTSC_PP    (1 << 9)
#define USBCMD_INTE  (1 << 2)

/* ---------- TRB-Struktur und Flags ---------- */

struct xhci_trb {
    uint32_t d0;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
} __attribute__((packed));

struct xhci_slot_context {
    uint32_t d[8];
} __attribute__((packed));

struct xhci_ep_context {
    uint32_t d[8];
} __attribute__((packed));

struct xhci_device_context {
    struct xhci_slot_context slot;
    struct xhci_ep_context   ep0;
} __attribute__((packed));

struct xhci_input_control_ctx {
    uint32_t drop_flags;
    uint32_t add_flags;
    uint32_t rsvd[6];
} __attribute__((packed));

struct xhci_input_context {
    struct xhci_input_control_ctx ctrl;
    struct xhci_slot_context      slot;
    struct xhci_ep_context        ep0;
} __attribute__((packed));

struct xhci_erst_entry {
    uint64_t base;
    uint32_t size;
    uint32_t rsvd;
} __attribute__((packed));

struct xhci_erst_entry g_erst;

/* ---------- Globale XHCI-Variablen ---------- */

uintptr_t g_xhci_mmio = 0;

volatile uint32_t* xhci_cap = 0;
volatile uint32_t* xhci_op  = 0;
uint32_t ep0_index = 0;

struct xhci_trb cmd_ring[64];
uint8_t cmd_cycle = 1;
uint32_t cmd_ring_index = 0;

struct xhci_device_context g_dev_ctx;
struct xhci_input_context  g_input_ctx;
uint64_t                   g_dcbaa[256];
int                        g_slot_id = 1; // erstmal 1 annehmen

/* EP0-Transfer-Ring und Event-Ring statisch */

struct xhci_trb ep0_ring[64];
uint8_t ep0_cycle = 1;

struct xhci_trb event_ring[64];
uint8_t event_cycle = 1;

/* ---------- Register-Flags ---------- */


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
    volatile uint32_t* doorbell = (volatile uint32_t *)(g_xhci_mmio + dboff);
    doorbell[g_slot_id] = 1; 
}

static void xhci_reset() {
    xhci_op[0] |= USBCMD_HCRST;
    while (xhci_op[1] & USBSTS_CNR) {}
}

static void xhci_start() {
    xhci_op[0] |= USBCMD_RUN | USBCMD_INTE;
}

static void xhci_push_cmd_trb(struct xhci_trb* trb)
{
    cmd_ring[cmd_ring_index] = *trb;
    cmd_ring[cmd_ring_index].d3 |= cmd_cycle;

    cmd_ring_index++;
    if (cmd_ring_index >= 64) {
        cmd_ring_index = 0;
        cmd_cycle ^= 1;
    }

    // Doorbell 0 für Command Ring
    uint32_t dboff = xhci_cap[5] & ~0x3;
    volatile uint32_t* doorbell = (volatile uint32_t *)(g_xhci_mmio + dboff);
    doorbell[0] = 0;
}

static int xhci_enable_slot()
{
    struct xhci_trb trb;
    memset(&trb, 0, sizeof(trb));

    trb.d3 = (9 << 10); // TRB Type = Enable Slot

    xhci_push_cmd_trb(&trb);

    // TODO: hier später auf Event warten
    return 1; // Slot-ID 1 annehmen (QEMU gibt meist 1 zurück)
}

static int xhci_address_device() {
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));

    g_input_ctx.ctrl.add_flags = (1 << 0) | (1 << 1); 

    g_input_ctx.slot.d[0] = 0; 
    g_input_ctx.slot.d[1] = 0; 

    uint64_t ep0_ring_ptr = (uint64_t)(uintptr_t)ep0_ring;

    g_input_ctx.ep0.d[0] = (4 << 10);          // Endpoint Type = Control (4)
    g_input_ctx.ep0.d[1] = (64 << 16);        // MaxPacketSize = 64
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

static int xhci_wait_for_event() {

    // Runtime Register Base holen
    uint32_t rts_offset = xhci_cap[2];
    volatile uint32_t* xhci_rt = (volatile uint32_t*)(g_xhci_mmio + (rts_offset << 5));

    // Interrupter 0 Register Set
    volatile uint32_t* ir0 = (volatile uint32_t*)((uintptr_t)xhci_rt + 0x20);

    // ERDP aus IMAN/ERDP-Register holen
    uint64_t erdp = ((uint64_t)ir0[7] << 32) | ir0[6];

    // Adresse ohne Flags (unterste 4 Bits sind Status)
    struct xhci_trb* ev = (struct xhci_trb*)(erdp & ~0xF);

    for (uint32_t i = 0; i < 100000000; i++) {

        uint32_t type  = (ev->d3 >> 10) & 0x3F;
        uint32_t cycle = ev->d3 & 1;

        if (type != 0 && cycle == event_cycle) {
            return 0;
        }
    }

    vga_printf(WHITE "<" RED " ERROR " WHITE "> Timeout\n");
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

    g_xhci_mmio = (uintptr_t)(bar0 & ~0xF);

    // mapping
    for (uint64_t addr = g_xhci_mmio; addr < g_xhci_mmio + 0x10000; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    }

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "XHCI Mapped!\n");

    xhci_cap = (volatile uint32_t*)g_xhci_mmio;

    uint8_t cap_length = *((volatile uint8_t*)xhci_cap);
    xhci_op = (volatile uint32_t*)(g_xhci_mmio + cap_length);

    uint32_t rts_offset = xhci_cap[2];       
    volatile uint32_t* xhci_rt = (volatile uint32_t*)(g_xhci_mmio + (rts_offset & ~0x1F));


    xhci_reset();
    xhci_start();

    memset(cmd_ring, 0, sizeof(cmd_ring));
    cmd_cycle = 1;
    cmd_ring_index = 0;

    // Command Ring Pointer setzen
    volatile uint32_t* xhci_doorbell = (volatile uint32_t*)(g_xhci_mmio + 0x20); // DB-Register-Basis
    volatile uint32_t* xhci_crcr = xhci_op + 0x18; // Command Ring Control Register

    uintptr_t cr_ptr = (uintptr_t)&cmd_ring[0];

    xhci_crcr[0] = (uint32_t)cr_ptr | cmd_cycle;
    xhci_crcr[1] = (uint32_t)(cr_ptr >> 32);

    memset(&g_dev_ctx, 0, sizeof(g_dev_ctx));
    memset(&g_input_ctx, 0, sizeof(g_input_ctx));
    memset(g_dcbaa, 0, sizeof(g_dcbaa));

    // DCBAA[slot_id] zeigt auf Device Context
    g_dcbaa[g_slot_id] = (uint64_t)(uintptr_t)&g_dev_ctx;

    // DCBAA in die Operational Registers schreiben (DCBAAP)
    volatile uint32_t* dcbaap = xhci_op + 0x30; // DCBAAP low/high
    uint64_t dcbaap_ptr = (uint64_t)(uintptr_t)g_dcbaa;

    dcbaap[0] = (uint32_t)dcbaap_ptr;
    dcbaap[1] = (uint32_t)(dcbaap_ptr >> 32);

    memset(ep0_ring, 0, sizeof(ep0_ring));
    ep0_cycle = 1;

    ep0_ring[63].d0 = (uint32_t)(uintptr_t)ep0_ring;
    ep0_ring[63].d1 = (uint32_t)((uint64_t)ep0_ring >> 32);
    ep0_ring[63].d2 = 0;
    ep0_ring[63].d3 = (TRB_TYPE_LINK << 10) | ep0_cycle;

    memset(event_ring, 0, sizeof(event_ring));
    event_cycle = 1;

    event_ring[63].d0 = (uint32_t)(uintptr_t)event_ring;
    event_ring[63].d1 = (uint32_t)((uint64_t)event_ring >> 32);
    event_ring[63].d2 = 0;
    event_ring[63].d3 = (TRB_TYPE_LINK << 10) | event_cycle;


    memset(&g_erst, 0, sizeof(g_erst));
    g_erst.base = (uint64_t)(uintptr_t)event_ring;
    g_erst.size = 64; // 64 TRBs
    g_erst.rsvd = 0;


    int slot_id = xhci_enable_slot();
    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> XHCI Slot enabled: %d\n", slot_id);
    
    /*
    // debug test
    vga_printf("MMIO low=%x high=%x\n",
        (uint32_t)g_xhci_mmio,
        (uint32_t)(g_xhci_mmio >> 32));
    vga_printf("RTSOFF=%x\n", xhci_cap[2]);

    vga_printf("RT=%x\n", (uint32_t)rt);

    vga_printf("IR=%x\n", (uint32_t)ir);


    vga_printf("vor ir0\n");
    vga_printf("0x%x\n", ir0[0]);
    vga_printf("nach ir0\n");
    */

    // vga_printf("bar0=%x\n", bar0);
    // vga_printf("bar1=%x\n", bar1);

    vga_printf("CAP0=%x\n", xhci_cap[0]);
    vga_printf("CAP1=%x\n", xhci_cap[1]);
    vga_printf("CAP2=%x\n", xhci_cap[2]);
    vga_printf("CAP3=%x\n", xhci_cap[3]);

    uintptr_t rt = g_xhci_mmio + (xhci_cap[2] & ~0x1F);
    uintptr_t ir = rt + 0x20;

    vga_printf("caplen=%x\n", *((volatile uint8_t*)g_xhci_mmio));

    for (int i = 0; i < 8; i++) {
        vga_printf("[%d] = %x\n", i, xhci_cap[i]);
    }

    volatile uint32_t* ir0 = (volatile uint32_t*)ir;

    // volatile uint32_t* ir0 = (volatile uint32_t*)((uintptr_t)xhci_rt + 0x20);

    // vga_printf("Test1\n");
    // vga_printf("0x%x\n", ir0[0]);

    // ERSTSZ
    ir0[2] = 1; 

    // vga_printf("Test2\n");

    // ERSTBA (low/high)
    uint64_t erst_ptr = (uint64_t)(uintptr_t)&g_erst;
    ir0[4] = (uint32_t)erst_ptr;
    ir0[5] = (uint32_t)(erst_ptr >> 32);

    // vga_printf("Test3\n");

    uint64_t erdp = (uint64_t)(uintptr_t)event_ring;
    ir0[6] = (uint32_t)erdp;
    ir0[7] = (uint32_t)(erdp >> 32);

    // vga_printf("Test4\n");

    ir0[1] |= 1;
    ir0[0] |= 1;
    ir0[1] = 0;

    // vga_printf("Test5\n");

    vga_printf("event_ring[0]: d0=%x d1=%x d2=%x d3=%x\n",
           event_ring[0].d0,
           event_ring[0].d1,
           event_ring[0].d2,
           event_ring[0].d3);

    if (xhci_wait_for_event() < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> " "Enable Slot failed\n");
    }

    xhci_address_device();

    if (xhci_wait_for_event() < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> " "Address Device failed\n");
    }

    vga_printf("event_ring[0]: d0=%x d1=%x d2=%x d3=%x\n",
           event_ring[0].d0,
           event_ring[0].d1,
           event_ring[0].d2,
           event_ring[0].d3);

    // vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> XHCI device addressed.\n");

    return 0;
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

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> Scanning XHCI ports...\n");
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

struct usb_setup_packet {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));

int xhci_control_transfer(
    int port,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    void* buffer
) {
    (void)port; // wir nehmen Slot 1 / EP0 fest an

    ep0_index = 0;

    struct usb_setup_packet setup;
    setup.bmRequestType = request_type;
    setup.bRequest      = request;
    setup.wValue        = value;
    setup.wIndex        = index;
    setup.wLength       = length;

    struct xhci_trb trb;

    // 1) Setup Stage TRB (Immediate Data)
    memset(&trb, 0, sizeof(trb));
    memcpy(&trb.d0, &setup, sizeof(setup)); // 8 Bytes Setup in d0/d1
    trb.d2 = sizeof(setup);
    trb.d3 = (TRB_TYPE_SETUP_STAGE << 10) | TRB_IDT; // | TRB_CYCLE;

    xhci_ep0_push_trb(&trb);

    // 2) Data Stage TRB (IN, falls length > 0)
    if (length > 0) {
        memset(&trb, 0, sizeof(trb));
        uint64_t buf = (uint64_t)(uintptr_t)buffer;

        trb.d0 = (uint32_t)buf;
        trb.d1 = (uint32_t)(buf >> 32);
        trb.d2 = length;
        trb.d3 = (TRB_TYPE_DATA_STAGE << 10) | (1 << 16); // IN

        xhci_ep0_push_trb(&trb);
    }


    // 3) Status Stage TRB (OUT, no data)
    memset(&trb, 0, sizeof(trb));
    trb.d3 = (TRB_TYPE_STATUS_STAGE << 10); // | TRB_CYCLE;

    xhci_ep0_push_trb(&trb);

    // 4) Auf Event warten (sehr simpel)
    if (xhci_wait_for_event() < 0)
        return -1;

    return 0;
}

int xhci_get_device_descriptor(int port, void* buffer, int max_len) {

    struct usb_setup_packet setup;
    setup.bmRequestType = 0x80; // Device-to-host, Standard, Device
    setup.bRequest      = 6;    // GET_DESCRIPTOR
    setup.wValue        = (1 << 8); // Descriptor Type = Device
    setup.wIndex        = 0;
    setup.wLength       = max_len;

    return xhci_control_transfer(
        port,
        setup.bmRequestType,
        setup.bRequest,
        setup.wValue,
        setup.wIndex,
        setup.wLength,
        buffer
    );
}

/* ---------- SCSI + BOT ---------- */

struct scsi_read10_cdb {
    uint8_t  opcode;      // 0x28
    uint8_t  flags;
    uint32_t lba;         // Big Endian
    uint8_t  reserved;
    uint16_t transfer_len;// Big Endian
    uint8_t  control;
} __attribute__((packed));

static uint32_t be32(uint32_t v) {
    return ((v & 0x000000FF) << 24) |
           ((v & 0x0000FF00) << 8)  |
           ((v & 0x00FF0000) >> 8)  |
           ((v & 0xFF000000) >> 24);
}

static uint16_t be16(uint16_t v) {
    return (uint16_t)(((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8));
}

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

/* Dummy-Endpoints – später aus Config-Descriptor setzen */
uint8_t g_msc_bulk_out_ep = 0x01;
uint8_t g_msc_bulk_in_ep  = 0x81;

/* Bulk-Stubs – nur damit alles linkt */

int xhci_bulk_out(uint8_t ep, const void* data, uint32_t len) {
    (void)ep;
    (void)data;
    (void)len;
    return -1; // TODO: echten XHCI-Bulk-OUT
}

int xhci_bulk_in(uint8_t ep, void* buffer, uint32_t len) {
    (void)ep;
    (void)buffer;
    (void)len;
    return -1; // TODO: echten XHCI-Bulk-IN
}

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

/* ---------- Descriptor-Strukturen + Mass-Storage-Init ---------- */

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

int xhci_get_config_descriptor(int port, void* buffer, int max_len) {

    struct usb_setup_packet setup;
    setup.bmRequestType = 0x80;      // Device-to-host, Standard, Device
    setup.bRequest      = 6;         // GET_DESCRIPTOR
    setup.wValue        = (2 << 8);  // Configuration Descriptor
    setup.wIndex        = 0;
    setup.wLength       = max_len;

    return xhci_control_transfer(
        port,
        setup.bmRequestType,
        setup.bRequest,
        setup.wValue,
        setup.wIndex,
        setup.wLength,
        buffer
    );
}

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
