#ifndef SYSCALL_HANDLER_H
#define SYSCALL_HANDLER_H

#include <stdint.h>
#include <stddef.h>

#include <fat.h>

#define MAX_FD 32

typedef enum { FD_NONE, FD_FILE, FD_DEVICE } FDType;

typedef enum { DEV_NONE, DEV_KEYBOARD, DEV_VGA } DeviceType;

typedef struct {
    FDType     type;    
    void      *object;   
    uint32_t   pos;      
} FileDescriptor;

extern FileDescriptor fd_table[MAX_FD];

extern FAT32_BootSector bpb;
extern uint32_t *fat_ram;
extern uint8_t *dir_buf;

void init_std_fds(void);

uint64_t sys_read(uint64_t fd, void *buf, uint64_t count);
uint64_t sys_write(uint64_t fd, const void *buf, uint64_t count);
uint64_t sys_open(const char *filename11, uint64_t flags);
uint64_t sys_close(uint64_t fd);
uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence);
void sys_exit(uint64_t code);
void sys_yield(void);

uint64_t syscall_handler(uint64_t rax, uint64_t rdi, uint64_t rsi,
                         uint64_t rdx, uint64_t r10, uint64_t r8, uint64_t r9);

#endif 
