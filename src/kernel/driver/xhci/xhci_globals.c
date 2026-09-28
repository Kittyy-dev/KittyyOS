#include <xhci.h>
#include <stdint.h>

uintptr_t g_xhci_mmio = 0;

volatile uint32_t* xhci_cap = 0;
volatile uint32_t* xhci_op  = 0;
volatile uint32_t* xhci_rt = 0;

uint32_t ep0_index = 0;

struct xhci_trb cmd_ring[64] __attribute__((aligned(64)));
uint8_t cmd_cycle = 1;
uint32_t cmd_ring_index = 0;

struct xhci_device_context g_dev_ctx __attribute__((aligned(64)));
struct xhci_input_context  g_input_ctx __attribute__((aligned(64)));

uint64_t g_dcbaa[256] __attribute__((aligned(64)));

int g_slot_id = 0;

struct xhci_trb ep0_ring[64] __attribute__((aligned(64)));
uint8_t ep0_cycle = 1;

struct xhci_trb event_ring[64] __attribute__((aligned(64)));
uint8_t event_cycle = 1;

struct xhci_erst_entry g_erst __attribute__((aligned(64)));

struct xhci_trb *event_dequeue = 0;

struct xhci_trb *g_last_event = 0;