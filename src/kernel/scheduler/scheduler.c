#include "vfs.h"
#include <kprint.h>
#include <context.h>
#include <stdbool.h>
#include <task.h>
#include <scheduler.h>
#include <stdint.h>
#include <colors.h>
#include <stddef.h>
#include <proc.h>
#include <string.h>
#include <heap.h>
#include <e1000.h>
#include <net.h>
#include <atomic.h>
#include <spinlock.h>
#include <panic.h>

static volatile bool scheduler_ready = false;

static uint64_t next_ngid = 1;

static struct task **tasks = NULL;

static size_t task_capacity = 0;
static size_t task_count = 0;

static struct task *current = 0;
static struct task *ready_queue = 0;
static spinlock_t scheduler_lock = SPINLOCK_INIT;

static int dns_sent = 0;
static int ping_sent = 0;
static uint8_t resolved[4];

extern void jump_usermode(void);
extern const uint8_t gateway_ip[4];

static uint64_t scheduler_alloc_ngid(void) {
    return __atomic_fetch_add(&next_ngid, 1, __ATOMIC_RELAXED);
}

size_t scheduler_get_task_count(void) {
    return task_count;
}

size_t scheduler_get_task_capacity(void) {
    return task_capacity;
}

struct task *scheduler_get_task(size_t index) {
    if (index >= task_capacity) {
        return NULL;
    }

    return tasks[index];
}

void scheduler_init(void) {
    scheduler_ready = false;

    tasks = kmalloc(INITIAL_TASK_CAPACITY * sizeof(*tasks));

    if (!tasks) {
        panic("Scheduler: Cannot allocate task table");

        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    memset(tasks, 0, INITIAL_TASK_CAPACITY * sizeof(*tasks));

    task_capacity = INITIAL_TASK_CAPACITY;
    task_count = 0;
    current = NULL;
    ready_queue = NULL;
}

static void enqueue_locked(struct task *task) {
    task->state = TASK_READY;
    task->next = NULL;

    if (!ready_queue) {
        ready_queue = task;
        return;
    }

    struct task *t = ready_queue;
    while (t->next) {
        t = t->next;
    }

    t->next = task;
}

static struct task *dequeue_locked(void) {
    struct task *task = ready_queue;

    if (!task) {
        return NULL;
    }

    ready_queue = task->next;
    task->next = NULL;

    return task;
}

static void scheduler_switch_next(void) {
    irq_flags_t flags;
    struct task *prev;
    struct task *next;

    spin_lock_irqsave(&scheduler_lock, flags);

    prev = current;

    if (prev) {
        enqueue_locked(prev);
    }

    next = dequeue_locked();

    if (!next) {
        if (prev) {
            prev->state = TASK_RUNNING;
        }

        spin_unlock_irqrestore(&scheduler_lock, flags);
        return;
    }

    current = next;
    next->state = TASK_RUNNING;

    if (prev && prev != next) {
        prev->voluntary_ctxt_switches++;
    }

    spin_unlock_irqrestore(&scheduler_lock, flags);

    if (prev != next) {
        context_switch(prev, next);
    }
}

void schedule(void) {
    scheduler_switch_next();
}

void yield(void) {
    scheduler_switch_next();
}

static void scheduler_preempt(void) {
    irq_flags_t flags;
    struct task *prev;
    struct task *next;

    spin_lock_irqsave(&scheduler_lock, flags);

    prev = current;

    if (!prev) {
        next = dequeue_locked();

        if (next) {
            current = next;
            next->state = TASK_RUNNING;
        }

        spin_unlock_irqrestore(&scheduler_lock, flags);

        if (next) {
            context_switch(NULL, next);
        }

        return;
    }

    if (!ready_queue) {
        spin_unlock_irqrestore(&scheduler_lock, flags);
        return;
    }

    enqueue_locked(prev);
    next = dequeue_locked();

    current = next;
    next->state = TASK_RUNNING;
    prev->nonvoluntary_ctxt_swichtes++;

    spin_unlock_irqrestore(&scheduler_lock, flags);

    context_switch(prev, next);
}

void scheduler_tick(void) {
    if (!scheduler_ready) {
        return;
    }

    scheduler_preempt();
}

__attribute__((noreturn))
void task_exit(void) {
    irq_flags_t flags;

    spin_lock_irqsave(&scheduler_lock, flags);

    struct task *old = current;
    struct task *next = dequeue_locked();

    if (!old) {
        spin_unlock_irqrestore(&scheduler_lock, flags);

        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    old->state = TASK_ZOMBIE;

    if (!next) {
        current = NULL;
        spin_unlock_irqrestore(&scheduler_lock, flags);

        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    current = next;
    next->state = TASK_RUNNING;

    spin_unlock_irqrestore(&scheduler_lock, flags);

    context_switch(old, next);

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static int scheduler_grow_task_table(void) {
    size_t new_capacity = task_capacity ? task_capacity * 2 : INITIAL_TASK_CAPACITY;

    if (new_capacity < task_capacity ||
        new_capacity > SIZE_MAX / sizeof(*tasks)) {
        return -1;
    }

    struct task **new_tasks = kmalloc(new_capacity * sizeof(*new_tasks));

    if (!new_tasks) {
        return -1;
    }

    memset(new_tasks, 0, new_capacity * sizeof(*new_tasks));

    memcpy(new_tasks, tasks, task_capacity * sizeof(*tasks));

    kfree(tasks);

    tasks = new_tasks;
    task_capacity = new_capacity;

    return 0;
}

void scheduler_reap_zombies(void) {
    irq_flags_t flags;

    for (size_t i = 0; i < task_capacity; i++) {
        spin_lock_irqsave(&scheduler_lock, flags);

        struct task *task = tasks[i];

        if (!task ||
            task->state != TASK_ZOMBIE ||
            task == current) {
            spin_unlock_irqrestore(&scheduler_lock, flags);
            continue;
        }

        void *stack = (void *)(uintptr_t)task->kernel_stack;

        task->kernel_stack = 0;
        task->rsp = 0;
        task->state = TASK_UNUSED;

        if (task_count > 0) {
            task_count--;
        }

        spin_unlock_irqrestore(&scheduler_lock, flags);

        if (stack) {
            kfree(stack);
        }
    }
}

struct task *current_task_get(void) {
    return current;
}

static struct scheduler_ops ops = {
    .init = scheduler_init,
    .schedule = schedule,
    .current_task = current_task_get,
};

static struct task *scheduler_alloc_task_slot(void) {
    for (size_t i = 0; i < task_capacity; i++) {
        if (tasks[i] && tasks[i]->state == TASK_UNUSED) {
            struct task *task = tasks[i];

            memset(task, 0, sizeof(*task));
            task->id = i;
            task->state = TASK_UNUSED;
        }

        if (!tasks[i]) {
            struct task *task = kmalloc(sizeof(*task));

            if (!task) {
                return NULL;
            }

            memset(task, 0, sizeof(*task));
            task->id = i;
            task->state = TASK_UNUSED;

            tasks[i] = task;
            return task;
        }
    }

    size_t old_capacity = task_capacity;

    if (scheduler_grow_task_table() != 0) {
        return NULL;
    }

    struct task *task = kmalloc(sizeof(*task));

    if (!task) {
        return NULL;
    }

    memset(task, 0, sizeof(*task));
    task->id = old_capacity;
    task->state = TASK_UNUSED;

    tasks[old_capacity] = task;

    return task;
}

struct task *task_create_name(task_entry_t entry, const char *name) {
    if (!entry) {
        return NULL;
    }

    struct task *task = scheduler_alloc_task_slot();

    if (!task) {
        return NULL;
    }

    task->tgid = task->id;
    task->state = TASK_READY;
    task->entry = entry;
    task->next = NULL;

    task->parent_id = current ? current->id : 0;
    task->uid = current ? current->uid : 0;
    task->gid = current ? current->gid : 0;
    task->umask = current ? current->umask : 0022;

    task->ngid = scheduler_alloc_ngid();
    task->voluntary_ctxt_switches = 0;
    task->nonvoluntary_ctxt_swichtes = 0;

    if (name) {
        strncpy(task->name, name, sizeof(task->name) - 1);
    } else {
        strncpy(task->name, "unknown", sizeof(task->name) - 1);
    }

    task->name[sizeof(task->name) - 1] = '\0';

    uint8_t *stack = kmalloc(TASK_STACK_SIZE);

    if (!stack) {
        task->state = TASK_UNUSED;
        return NULL;
    }

    uint64_t *sp = (uint64_t *)(stack + TASK_STACK_SIZE);

    *(--sp) = (uint64_t)task->entry;
    *(--sp) = (uint64_t)task_trampoline;

    *(--sp) = 0; // rbx
    *(--sp) = 0; // rbp
    *(--sp) = 0; // r12
    *(--sp) = 0; // r13
    *(--sp) = 0; // r14
    *(--sp) = 0; // r15

    task->kernel_stack = (uint64_t)stack;
    task->rsp = (uint64_t)sp;

    irq_flags_t flags;
    spin_lock_irqsave(&scheduler_lock, flags);

    enqueue_locked(task);
    task_count++;

    spin_unlock_irqrestore(&scheduler_lock, flags);

    return task;
}

struct task *task_create_user(task_entry_t entry, const char *name, uint64_t uid, uint64_t gid, uint16_t umask) {
    struct task *task = task_create_name(entry, name);

    if (!task) {
        return NULL;
    }

    task->uid = uid;
    task->gid = gid;
    task->umask = umask;

    return task;
}

struct task *task_create(task_entry_t entry) {
    return task_create_name(entry, "unknown");
}

void proc_fs_start() {
    proc_init();

    while (1) {
        scheduler_reap_zombies();
        yield();
    }
}

void network_driver() {
    log_kernel("Starting Network driver!\n");

    vfs_node_t *dir = vfs_resolve_path("/"); // /proc/17294103305076670291
    
    vfs_ls(dir);

    while (1) {
        e1000_poll_rx();

        /*
        if (!dns_sent && arp_gateway_ready()) {
            if (dns_resolve("google.com")) {
                dns_sent = 1;
            }
        }

        if (dns_sent && !ping_sent && dns_get_result(resolved)) {
            if (icmp_ping(resolved)) {
                ping_sent = 1;
            }
        } */

        yield();
    }
}

void scheduler_start(void) {
    log_kernel("Scheduler loaded!\n");
    register_scheduler(&ops);

    scheduler_init();

    scheduler_ready = true;

    task_create_name(proc_fs_start, "ProcFS");
    task_create_name(network_driver, "Network");
}


_Static_assert(offsetof(struct task, rsp) == 0x10, "struct task: rsp offset mismatch");