#include <scheduler.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static struct scheduler_ops *scheduler = NULL;

void register_scheduler(struct scheduler_ops *ops) {
    scheduler = ops;
}