#include <paging.h>
#include <pci.h>
#include <e1000.h>
#include <stdint.h>
#include <kprint.h>
#include <colors.h>
#include <net.h>

volatile uint32_t *e1000_regs = 0;
static uint8_t e1000_mac[6] = {0};
static int e1000_mac_valid = 0;

int e1000_map_mmio(void) {
    if (!pci_network_found ||
        pci_network_vendor != 0x8086 ||
        pci_network_device != 0x100E) {
        log_error("e1000: Device not found!\n");
        return 0;
    }

    uint64_t bar = pci_get_bar0(
        pci_network_bus,
        pci_network_slot,
        pci_network_func
    );

    if (bar == 0 || (bar & 1)) {
        log_error("e1000: Invalid BAR0!\n");
        return 0;
    }

    uint64_t phys_start = bar & ~0xFFFULL;
    uint64_t virt_start = HHDM_BASE + phys_start;

    // Map MMIO registers as uncached
    for (uint64_t offset = 0; offset < E1000_MMIO_SIZE; offset += 0x1000) {
        map_page(
            virt_start + offset,
            phys_start + offset,
            PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT
        );
    }

    e1000_regs = (volatile uint32_t *)(uintptr_t)(
        virt_start + (bar & 0xFFFULL)
    );

    log_kernel("e1000: BAR0 physical: %08x\n", (unsigned int)bar);
    log_kernel("e1000: MMIO mapped successfully\n");

    return 1;
}

static inline uint32_t e1000_read(uint32_t reg) {
    return e1000_regs[reg / 4];
}

static inline void e1000_write(uint32_t reg, uint32_t value) {
    e1000_regs[reg / 4] = value;
}

int e1000_reset(void) {
    uint32_t ctrl = e1000_read(E1000_REG_CTRL);

    e1000_write(E1000_REG_CTRL, ctrl | E1000_CTRL_RST);

    // Wait for reset
    for (volatile unsigned int i = 0; i < 1000000; i++) {
        asm volatile("pause");
    }

    uint32_t status = e1000_read(E1000_REG_STATUS);

    return 1;
}

void e1000_test_mmio(void) {
    if (!e1000_regs) {
        log_error("e1000: MMIO pointer is NULL!\n");
        return;
    }
}

void e1000_read_mac(void) {
    uint32_t ral = e1000_read(E1000_REG_RAL0);
    uint32_t rah = e1000_read(E1000_REG_RAH0);

    if (!(rah & E1000_RAH_AV)) {
        e1000_mac_valid = 0;
        log_error("e1000: RAH0 address not valid!\n");
        return;
    }

    e1000_mac[0] = (uint8_t)(ral);
    e1000_mac[1] = (uint8_t)(ral >> 8);
    e1000_mac[2] = (uint8_t)(ral >> 16);
    e1000_mac[3] = (uint8_t)(ral >> 24);
    e1000_mac[4] = (uint8_t)(rah);
    e1000_mac[5] = (uint8_t)(rah >> 8);

    e1000_mac_valid = 1;

    log_kernel("e1000: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned int)e1000_mac[0], (unsigned int)e1000_mac[1], (unsigned int)e1000_mac[2], (unsigned int)e1000_mac[3], (unsigned int)e1000_mac[4], (unsigned int)e1000_mac[5]);
}

const uint8_t *e1000_get_mac(void) {
    if (!e1000_mac_valid) {
        return 0;
    }

    return e1000_mac;
}

static e1000_tx_desc_t tx_descs[E1000_TX_DESC_COUNT] 
    __attribute__((aligned(128)));

static uint8_t tx_buffers[E1000_TX_DESC_COUNT][2048] 
    __attribute__((aligned(16)));

static uint32_t tx_tail = 0;

void e1000_init_tx(void) {
    for (uint32_t i = 0; i < E1000_TX_DESC_COUNT; i++) {
        tx_descs[i].addr = virt_to_phys((uint64_t)(uintptr_t)tx_buffers[i]);
        tx_descs[i].length = 0;
        tx_descs[i].cmd = 0;
        tx_descs[i].status = E1000_TX_STATUS_DD;
    }

    uint64_t ring_phys = virt_to_phys((uint64_t)(uintptr_t)tx_descs);

    e1000_write(E1000_REG_TDBAL, (uint32_t)ring_phys);
    e1000_write(E1000_REG_TDBAH, (uint32_t)(ring_phys >> 32));
    e1000_write(E1000_REG_TDLEN, sizeof(tx_descs));
    e1000_write(E1000_REG_TDH, 0);
    e1000_write(E1000_REG_TDT, 0);

    e1000_write(E1000_REG_TIPG, 10 | (10 << 10) | (10 << 20));

    e1000_write(E1000_REG_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (15u << 4) | (64u << 12));

    tx_tail = 0;

    log_kernel("e1000: TX initialized!\n");
}

static e1000_rx_desc_t rx_descs[E1000_RX_DESC_COUNT]
    __attribute__((aligned(128)));

static uint8_t rx_buffers[E1000_RX_DESC_COUNT][2048]
    __attribute__((aligned(16)));

static uint32_t rx_next = 0;

void e1000_init_rx(void) {
    for (uint32_t i = 0; i < E1000_RX_DESC_COUNT; i++) {
        rx_descs[i].addr = virt_to_phys((uint64_t)(uintptr_t)rx_buffers[i]);
        rx_descs[i].status = 0;
        rx_descs[i].errors = 0;
    }

    uint64_t ring_phys = virt_to_phys((uint64_t)(uintptr_t)rx_descs);

    e1000_write(E1000_REG_RDBAL, (uint32_t)ring_phys);
    e1000_write(E1000_REG_RDBAH, (uint32_t)(ring_phys >> 32));
    e1000_write(E1000_REG_RDLEN, sizeof(rx_descs));
    e1000_write(E1000_REG_RDH, 0);
    e1000_write(E1000_REG_RDT, E1000_RX_DESC_COUNT - 1);

    e1000_write(E1000_REG_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC | (0u << 16));

    rx_next = 0;

    log_kernel("e1000: RX initialized!\n");
}

int e1000_send(const void *data, uint16_t length) {
    if (!data || length == 0 || length > 2048) {
        return 0;
    }

    uint32_t index = tx_tail;
    e1000_tx_desc_t *desc = &tx_descs[index];

    if (!(desc->status & E1000_TX_STATUS_DD)) {
        return 0;
    }

    const uint8_t *src = (const uint8_t *)data;

    for (uint16_t i = 0; i < length; i++) {
        tx_buffers[index][i] = src[i];
    }

    desc->length = length;
    desc->cmd = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    desc->status = 0;

    asm volatile("sfence" ::: "memory");

    tx_tail = (tx_tail + 1) % E1000_TX_DESC_COUNT;
    e1000_write(E1000_REG_TDT, tx_tail);

    return 1;
}

void e1000_poll_rx(void) {
    e1000_rx_desc_t *desc = &rx_descs[rx_next];

    if (!(desc->status & E1000_RX_STATUS_DD)) {
        return;
    }

    if (desc->errors == 0 && desc->length >= 14) {
        ethernet_receive(rx_buffers[rx_next], desc->length);
    }

    desc->status = 0;
    desc->errors = 0;

    uint32_t old_index = rx_next;
    rx_next = (rx_next + 1) % E1000_RX_DESC_COUNT;

    asm volatile("sfence" ::: "memory");

    e1000_write(E1000_REG_RDT, old_index);
}

/*
void e1000_debug_rings(void) {
    log_kernel("e1000: TDH=%u TDT=%u\n",
        (unsigned int)e1000_read(E1000_REG_TDH),
        (unsigned int)e1000_read(E1000_REG_TDT));

    log_kernel("e1000: RDH=%u RDT=%u\n",
        (unsigned int)e1000_read(E1000_REG_RDH),
        (unsigned int)e1000_read(E1000_REG_RDT));

    log_kernel("e1000: TX desc0 status=%02x\n",
        (unsigned int)tx_descs[0].status);

    log_kernel("e1000: RX desc0 status=%02x\n",
        (unsigned int)rx_descs[0].status);
} */

void e1000_test_send(void) {
    uint8_t frame[60] = {0};

    // Destination: Broadcast
    for (int i = 0; i < 6; i++) {
        frame[i] = 0xFF;
    }

    // Source: Our MAC
    const uint8_t mac[6] = {
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56
    };

    for (int i = 0; i < 6; i++) {
        frame[6 + i] = mac[i];
    }

    // EtherType 0x88B5 (experimental)
    frame[12] = 0x88;
    frame[13] = 0xB5;

    const char *msg = "Hello from my kernel!";

    for (int i = 0; msg[i] && i < 46; i++) {
        frame[14 + i] = (uint8_t)msg[i];
    }

    if (e1000_send(frame, sizeof(frame))) {
        log_kernel("e1000: Test frame queued\n");
    } else {
        log_error("e1000: Failed to queue test frame\n");
    }
}