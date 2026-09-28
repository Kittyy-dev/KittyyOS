// Opsec Level = Unter das OS

#include <mouse.h>
#include <ports.h>
#include <kprint.h>
#include <stdint.h>
#include <vbe.h>

#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60

// PS/2 Commands
#define PS2_CMD_ENABLE_AUX 0xA8
#define PS2_CMD_GET_CONFIG 0x20
#define PS2_CMD_SET_CONFIG 0X60
#define PS2_CMD_WRITE_AUX 0xD4

// Mouse Commands
#define MOUSE_CMD_RESET 0xFF
#define MOUSE_CMD_ENABLE 0XF4
#define MOUSE_CMD_SET_DEFAULTS 0XF6

#define MOUSE_ACK 0xFA

static volatile int mouse_x = 0;
static volatile int mouse_y = 0;

static volatile bool mouse_btn_left = false;
static volatile bool mouse_btn_right = false;
static volatile bool mouse_btn_middle = false;

static uint8_t mouse_packet[3];
static uint8_t mouse_cycle = 0;

static void mouse_wait_input(void) {
    int timeout = 100000;

    while (timeout--) {
        if (!(inb(PS2_STATUS) & 0x02)) {
            return;
        }
    }
}

static void mouse_wait_output(void) {
    int timeout = 100000;

    while (timeout--) {
        if (inb(PS2_STATUS) & 0x01) {
            return;
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait_input();

    outb(PS2_COMMAND, PS2_CMD_WRITE_AUX);

    mouse_wait_input();

    outb(PS2_DATA, data);
}

static uint8_t mouse_read(void) {
    mouse_wait_output();

    return inb(PS2_DATA);
}

static void mouse_set_config(uint8_t config) {
    mouse_wait_input();
    outb(PS2_COMMAND, PS2_CMD_SET_CONFIG);

    mouse_wait_input();
    outb(PS2_DATA, config);
}

static uint8_t mouse_get_config(void) {
    mouse_wait_input();
    outb(PS2_COMMAND, PS2_CMD_GET_CONFIG);

    mouse_wait_output();

    return inb(PS2_DATA);
}

void mouse_init(void)
{
    kprintf("mouse: init\n");

    mouse_wait_input();
    outb(PS2_COMMAND, PS2_CMD_ENABLE_AUX);

    kprintf("mouse: aux enabled\n");

    uint8_t config = mouse_get_config();

    kprintf("mouse: config=%x\n", config);

    config |= 0x02;        // IRQ12
    config &= ~(1 << 5);   // Auxiliary clock enable

    mouse_set_config(config);

    kprintf("mouse: config set=%x\n", config);

    mouse_write(MOUSE_CMD_SET_DEFAULTS);

    uint8_t ack = mouse_read();

    kprintf("mouse: defaults ack=%x\n", ack);

    if (ack != MOUSE_ACK)
        return;

    mouse_write(MOUSE_CMD_ENABLE);

    ack = mouse_read();

    kprintf("mouse: enable ack=%x\n", ack);

    if (ack != MOUSE_ACK)
        return;

    mouse_x = 0;
    mouse_y = 0;
    mouse_cycle = 0;

    mouse_btn_left = false;
    mouse_btn_right = false;
    mouse_btn_middle = false;

    kprintf("mouse: READY\n");
}

void mouse_irq_handler(void) {
    kprintf("Mouse");

    uint8_t data = inb(PS2_DATA);

    if (mouse_cycle == 0) {
        if (!(data & 0x08)) {
            return;
        }

        mouse_packet[0] = data;
        mouse_cycle = 1;
        return;
    }

    if (mouse_cycle == 1) {
        mouse_packet[1] = data;
        mouse_cycle = 2;
        return; 
    }

    mouse_packet[2] = data;
    mouse_cycle = 0;

    uint8_t flags = mouse_packet[0];

    int8_t dx = (int8_t)mouse_packet[1];
    int8_t dy = (int8_t)mouse_packet[2];

    mouse_btn_left = flags & 0x01;
    mouse_btn_right = flags & 0x02;
    mouse_btn_middle = flags & 0x04;

    if (flags & 0x40) {
        dx = 0;
    }

    if (flags & 0x80) {
        dy = 0;
    }

    mouse_x += dx;
    mouse_y -= dy;

    if (mouse_x < 0) {
        mouse_x = 0;
    }

    if (mouse_y < 0) {
        mouse_y = 0;
    }

    if (mouse_x >= vbe.width) {
        mouse_x = vbe.width - 1;
    }

    if (mouse_y >= vbe.height) {
        mouse_y = vbe.height - 1;
    }
}

int mouse_get_x(void) {
    return mouse_x;
}

int mouse_get_y(void) {
    return mouse_y;
}

bool mouse_left(void) {
    return mouse_btn_left;
}

bool mouse_right(void) {
    return mouse_btn_right;
}

bool mouse_middle(void) {
    return mouse_btn_middle;
}