#include <context.h>
#include <scheduler.h>
#include <stdint.h>
#include <stddef.h>
#include <kernel_api.h>
#include <task.h>
#include <kprint.h>

static struct scheduler_ops *scheduler = NULL;

void register_scheduler(struct scheduler_ops *ops) {
    scheduler = ops;
}

void kernel_schedule(void) {
    if (!scheduler) {
        return;
    }

    if (!scheduler->schedule || !scheduler->current_task) {
        return;
    }

    // kprintf("TEST");

    struct task *old = scheduler->current_task();

    scheduler->schedule();

    struct task *new = scheduler->current_task();

    if (old != new && new) {
        kernel_context_switch(old, new);
    }
}