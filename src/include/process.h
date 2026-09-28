/*
KittyyOS Copyright(C)
Process Manager
*/

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <syscall_handler.h>

#define MAX_PROCESSES 16

typedef enum { PROC_UNUSED, PROC_READY, PROC_RUNNING, PROC_WAITING, PROC_ZOMBIE } ProcState;

typedef struct {
    uint32_t pid;
    ProcState state;
    uint64_t regs[16];
    void* stack;
    FileDescriptor fd_table[MAX_FD];
} Process;

extern Process process_table[MAX_PROCESSES];
extern Process* current_process;

void init_process_manager(void);
int create_process(void(*entry)(void));
void schedule(void);
void context_switch(Process* old, Process* new);

#endif