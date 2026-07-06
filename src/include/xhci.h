#ifndef XHCI_H
#define XHCI_H

#include <stdint.h>

/* ---------- TRB-Struktur ---------- */

struct xhci_trb {
    uint32_t d0;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
} __attribute__((packed));

/* ---------- Context-Strukturen ---------- */

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

/* ---------- TRB-Typen ---------- */

#define TRB_TYPE_SETUP_STAGE   2
#define TRB_TYPE_DATA_STAGE    3
#define TRB_TYPE_STATUS_STAGE  4
#define TRB_TYPE_EVENT_DATA    5
#define TRB_TYPE_LINK          6
#define TRB_TYPE_TRANSFER      32

/* ---------- TRB-Flags ---------- */

#define TRB_CYCLE  (1 << 0)
#define TRB_IOC    (1 << 5)
#define TRB_IDT    (1 << 6)

/* ---------- Register-Flags ---------- */

#define USBCMD_RUN   (1 << 0)
#define USBCMD_HCRST (1 << 1)
#define USBCMD_INTE  (1 << 2)

#define USBSTS_CNR   (1 << 11)

#define PORTSC_CCS   (1 << 0)
#define PORTSC_PED   (1 << 1)
#define PORTSC_PR    (1 << 4)
#define PORTSC_PP    (1 << 9)

/* ---------- Globale XHCI-Variablen (extern!) ---------- */

extern uintptr_t g_xhci_mmio;

extern volatile uint32_t* xhci_cap;
extern volatile uint32_t* xhci_op;

extern struct xhci_trb cmd_ring[64];
extern uint8_t cmd_cycle;
extern uint32_t cmd_ring_index;

extern struct xhci_trb ep0_ring[64];
extern uint8_t ep0_cycle;
extern uint32_t ep0_index;

extern struct xhci_trb event_ring[64];
extern uint8_t event_cycle;

extern struct xhci_device_context g_dev_ctx;
extern struct xhci_input_context  g_input_ctx;
extern uint64_t g_dcbaa[256];
extern int g_slot_id;

extern struct xhci_erst_entry g_erst;

/* ---------- Funktionsprototypen ---------- */

int xhci_init(void);
void xhci_irq_handler(void);

int xhci_control_transfer(
    int port,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    void* buffer
);

int xhci_get_device_descriptor(int port, void* buffer, int max_len);
int xhci_get_config_descriptor(int port, void* buffer, int max_len);

int xhci_init_mass_storage(int port);

int xhci_bulk_out(uint8_t ep, const void* data, uint32_t len);
int xhci_bulk_in(uint8_t ep, void* buffer, uint32_t len);

void xhci_enable_ports(void);
int xhci_find_device_port(void);
int xhci_scsi_read10(uint32_t lba, uint8_t count, uint8_t* buffer);


#endif
