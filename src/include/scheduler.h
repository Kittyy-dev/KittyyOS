#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

struct task;

struct scheduler_ops {
    void (*init)(void);
    void (*schedule)(void);
    struct task *(*current_task)(void);
};

int scheduler_get_task_count(void);
struct task *scheduler_get_tasks(void);

void register_scheduler(struct scheduler_ops *ops);
void kernel_schedule(void);

#endif