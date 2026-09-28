#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <kernel_api.h>

#define MAX_TASKS 64
#define TASK_STACK_SIZE 16384
#define TASK_NAME_MAX 16

extern void task_trampoline(void);

typedef enum {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
} task_state_t;

typedef void (*task_entry_t)(KernelAPI *api);

struct task {
    uint64_t id;
    task_state_t state;

    uint64_t rsp;
    uint64_t cr3;
    uint64_t kernel_stack;

    task_entry_t entry;
    KernelAPI *api;

    char name[32];

    uint64_t parent_id;
    uint64_t tgid;
    uint64_t ngid;

    uint64_t uid;
    uint64_t gid;
    uint16_t umask;

    uint64_t voluntary_ctxt_switches;
    uint64_t nonvoluntary_ctxt_swichtes;

    struct task *next;
};

struct context {
    uint64_t rsp;

    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

void task_init(struct task *task);

#endif