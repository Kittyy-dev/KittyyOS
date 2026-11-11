
#ifndef E1000_H
#define E1000_H

#include <stdint.h>

#define E1000_MMIO_SIZE 0x20000ULL
#define E1000_REG_CTRL    0x0000
#define E1000_REG_STATUS  0x0008
#define E1000_REG_EECD    0x0010
#define E1000_REG_RAL0    0x5400
#define E1000_REG_RAH0    0x5404

#define E1000_CTRL_RST   (1u << 26)
#define E1000_RAH_AV     (1u << 31)

#define E1000_REG_RCTL    0x0100
#define E1000_REG_TCTL    0x0400
#define E1000_REG_TIPG    0x0410

#define E1000_REG_RDBAL   0x2800
#define E1000_REG_RDBAH   0x2804
#define E1000_REG_RDLEN   0x2808
#define E1000_REG_RDH     0x2810
#define E1000_REG_RDT     0x2818

#define E1000_REG_TDBAL   0x3800
#define E1000_REG_TDBAH   0x3804
#define E1000_REG_TDLEN   0x3808
#define E1000_REG_TDH     0x3810
#define E1000_REG_TDT     0x3818

#define E1000_RCTL_EN     (1u << 1)
#define E1000_RCTL_BAM    (1u << 15)
#define E1000_RCTL_SECRC  (1u << 26)

#define E1000_TCTL_EN     (1u << 1)
#define E1000_TCTL_PSP    (1u << 3)

#define E1000_TX_CMD_EOP  (1u << 0)
#define E1000_TX_CMD_IFCS (1u << 1)
#define E1000_TX_CMD_RS   (1u << 3)

#define E1000_TX_STATUS_DD (1u << 0)
#define E1000_RX_STATUS_DD (1u << 0)
#define E1000_TX_DESC_COUNT 16
#define E1000_RX_DESC_COUNT 16

typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} e1000_rx_desc_t;

typedef struct __attribute__((packed)) {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} e1000_tx_desc_t;

int e1000_init(void);
int e1000_is_ready(void);
int e1000_link_up(void);

void e1000_print_mac(void);
void e1000_test_mmio(void);
int e1000_reset(void);
void e1000_read_mac(void);
const uint8_t *e1000_get_mac(void);
int e1000_map_mmio(void);
void e1000_init_tx(void);
void e1000_init_rx(void);
// void e1000_debug_rings(void);
void e1000_test_send(void);
void e1000_poll_rx(void);
int e1000_send(const void *data, uint16_t length);

#endif