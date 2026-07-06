#include <stdint.h>
#include <process.h>
#include <vga.h>

void userspace() {
    vga_print("[ KERNEL ] Starting userspace");
    vga_print("[ Userspace ] Userspace startet!\n");
}