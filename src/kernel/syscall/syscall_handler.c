#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vga.h>
#include <keyboard.h>
#include <fat.h>          
#include <syscall_handler.h>
#include <process.h>
#include <heap.h>

FileDescriptor fd_table[MAX_FD];

extern FAT32_BootSector bpb;
extern uint32_t *fat_ram;     
extern uint8_t *dir_buf;     

static int find_free_fd(void) {
    for (int i = 0; i < MAX_FD; i++) if (fd_table[i].type == FD_NONE) return i;
    return -1;
}

void init_std_fds(void) {
    fd_table[0].type   = FD_DEVICE;
    fd_table[0].object = (void*)(uintptr_t)DEV_KEYBOARD;
    fd_table[0].pos    = 0;

    fd_table[1].type   = FD_DEVICE;
    fd_table[1].object = (void*)(uintptr_t)DEV_VGA;
    fd_table[1].pos    = 0;

    fd_table[2].type   = FD_DEVICE;
    fd_table[2].object = (void*)(uintptr_t)DEV_VGA;
    fd_table[2].pos    = 0;

    for (int i = 3; i < MAX_FD; i++) {
        fd_table[i].type   = FD_NONE;
        fd_table[i].object = NULL;
        fd_table[i].pos    = 0;
    }
}

uint64_t sys_read(uint64_t fd, void *buf, uint64_t count) {
    if (fd >= MAX_FD) return (uint64_t)-1;
    FileDescriptor *d = &fd_table[fd];

    if (d->type == FD_DEVICE) {
        DeviceType dev = (DeviceType)(uintptr_t)d->object;
        if (dev == DEV_KEYBOARD) {
            char *cbuf = (char*)buf;
            for (uint64_t i = 0; i < count; i++) {
                cbuf[i] = get_char_from_keyboard();
            }
            return count;
        }
        return (uint64_t)-1;
    }

    if (d->type == FD_FILE) {
        FileHandle *fh = (FileHandle*)d->object;
        int n = read_file(*fh, &bpb, fat_ram, (uint8_t*)buf);
        if (n > 0) d->pos += (uint32_t)n;
        return (n >= 0) ? (uint64_t)n : (uint64_t)-1;
    }

    return (uint64_t)-1;
}

uint64_t sys_write(uint64_t fd, const void *buf, uint64_t count) {
    if (fd >= MAX_FD) return (uint64_t)-1;
    FileDescriptor *d = &fd_table[fd];

    if (d->type == FD_DEVICE) { 
        DeviceType dev = (DeviceType)(uintptr_t)d->object;
        if (dev == DEV_VGA) {
            const char *cbuf = (const char*)buf;
            for (uint64_t i = 0; i < count; i++) {
                vga_putc(cbuf[i]);
            }
            return count;
        }
        return (uint64_t)-1;
    }

    if (d->type == FD_FILE) {
        FileHandle *fh = (FileHandle*)d->object;
        int n = write_file(*fh, &bpb, fat_ram, buf, (uint32_t)count);
        if (n > 0) d->pos += (uint32_t)n;
        return (n >= 0) ? (uint64_t)n : (uint64_t)-1;
    }

    return (uint64_t)-1;
}

uint64_t sys_open(const char *filename11, uint64_t flags) {
    FileHandle fh = open_file(filename11, &bpb, fat_ram, dir_buf);
    if (fh.startCluster == 0) return (uint64_t)-1;

    int fd = find_free_fd();
    if (fd < 0) return (uint64_t)-1;

    FileHandle *p = (FileHandle*)kmalloc(sizeof(FileHandle));
    *p = fh;

    fd_table[fd].type   = FD_FILE;
    fd_table[fd].object = p;
    fd_table[fd].pos    = 0;
    return (uint64_t)fd;
}

uint64_t sys_close(uint64_t fd) {
    if (fd >= MAX_FD) return (uint64_t)-1;
    FileDescriptor *d = &fd_table[fd];

    if (d->type == FD_FILE && d->object) {
        kfree(d->object);
    }
    d->type   = FD_NONE;
    d->object = NULL;
    d->pos    = 0;
    return 0;
}

uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence) {
    if (fd >= MAX_FD) return (uint64_t)-1;
    FileDescriptor *d = &fd_table[fd];
    if (d->type != FD_FILE) return (uint64_t)-1;

    FileHandle *fh = (FileHandle*)d->object;
    uint64_t newpos = d->pos;

    if (whence == 0) newpos = offset;
    else if (whence == 1) newpos = d->pos + offset;
    else if (whence == 2) newpos = (uint64_t)fh->size + offset;
    else return (uint64_t)-1;

    if (newpos > fh->size) newpos = fh->size; 
    d->pos = (uint32_t)newpos;
    return d->pos;
}

void sys_exit(uint64_t code) {
    kprintf("Process exit: "); vga_putc('0' + (char)(code % 10)); vga_putc('\n');
    for (;;) {}
}

void sys_yield(void) {
    schedule();
}

uint64_t syscall_handler(uint64_t rax, uint64_t rdi, uint64_t rsi,
                         uint64_t rdx, uint64_t r10, uint64_t r8, uint64_t r9) {
    switch (rax) {
        case 0:  return sys_read(rdi, (void*)rsi, rdx);
        case 1:  return sys_write(rdi, (const void*)rsi, rdx);
        case 2:  return sys_open((const char*)rdi, rsi);
        case 3:  return sys_close(rdi);
        case 8:  return sys_lseek(rdi, rsi, rdx);
        case 60: sys_exit(rdi); return 0;
        default:
            kprintf("Unknown syscall\n");
            return (uint64_t)-1;
    }
}
