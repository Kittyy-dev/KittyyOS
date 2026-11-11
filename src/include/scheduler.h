#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

struct task;

struct scheduler_ops {
    void (*init)(void);
    void (*schedule)(void);
    struct task *(*current_task)(void);
};

size_t scheduler_get_task_count(void);

void register_scheduler(struct scheduler_ops *ops);
size_t scheduler_get_task_capacity(void);
struct task *scheduler_get_task(size_t index);
void scheduler_tick(void);

#endif