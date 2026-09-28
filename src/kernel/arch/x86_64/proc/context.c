#include "task.h"
#include <context.h>

void kernel_context_switch(struct task* old, struct task *new) {
    if (!old || !new || old == new) {
        return;
    }

    context_switch(old, new);
}