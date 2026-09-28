#include <context.h>
#include <kernel_api.h>
#include <task.h>
#include <scheduler.h>
#include <stdint.h>
#include <colors.h>
#include <stddef.h>

static uint64_t next_ngid = 1;

static struct task tasks[MAX_TASKS];

static struct task *current = 0;

static struct task *ready_queue = 0;

static KernelAPI *kernel_api = 0;

static KernelAPI *scheduler_api = 0;

extern bool kernel_load_user(const char *path, KernelAPI *api);

extern void proc_init(KernelAPI *api);
extern void shell(KernelAPI *api);

static uint64_t scheduler_alloc_ngid(void) {
    return next_ngid;
}

int scheduler_get_task_count(void) {
    return MAX_TASKS;
}

struct task* scheduler_get_tasks(void)
{
    return tasks;
}


void scheduler_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].id = i;
        tasks[i].state = TASK_UNUSED;
        tasks[i].next = 0;
    }

    current = 0;
    ready_queue = 0;
}

static void enqueue(struct task *task) {
    task->state = TASK_READY;

    task->next = 0;

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

static struct task *dequeue(void) {
    if (!ready_queue) {
        return 0;
    }

    struct task *task = ready_queue;

    ready_queue = ready_queue->next;

    task->next = 0;

    return task;
}

void schedule(void) {
    struct task *next = dequeue();

    if (!next) {
        return;
    }

    struct task *prev = current;

    current = next;
    current->state = TASK_RUNNING;

    if (prev) {
        enqueue(prev);
        context_switch(prev, next);
    } else {
        context_switch(NULL, next);
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

struct task *task_create_name(task_entry_t entry, const char *name) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_UNUSED) {
            continue;
        }

        struct task *task = &tasks[i];

        task->id = i;
        task->tgid = task->id;
        task->state = TASK_READY;
        task->entry = entry;
        task->api = kernel_api;
        task->next = NULL;

        task->parent_id = current ? current->id : 0;
        
        task->uid = current ? current->uid : 0;
        task->gid = current ? current->gid : 0;
        task->umask = current ? current->umask : 0022;

        task->ngid = scheduler_alloc_ngid();

        task->voluntary_ctxt_switches = 0;
        task->nonvoluntary_ctxt_swichtes = 0;

        if (name) {
            kernel_api->strncpy(
                task->name,
                name,
                sizeof(task->name) - 1
            );

            task->name[sizeof(task->name) - 1] = '\0';
        } else {
            kernel_api->strncpy(
                task->name,
                "unknown",
                sizeof(task->name) -1
            );

            task->name[sizeof(task->name) - 1] = '\0';
        }

        uint8_t *stack = kernel_api->kmalloc(TASK_STACK_SIZE);

        if (!stack)
            return NULL;

        uint64_t *sp =
            (uint64_t *)(stack + TASK_STACK_SIZE);

        *(--sp) = (uint64_t)task->api;
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

        enqueue(task);

        return task;
    }

    return NULL;
}

struct task *task_create_user(task_entry_t entry, const char* name, uint64_t uid, uint64_t gid, uint16_t umask) {
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

void yield(void) {
    struct task *prev = current;
    struct task *next = dequeue();

    if (!next) {
        return;
    }

    current = next;
    current->state = TASK_RUNNING;

    enqueue(prev);

    context_switch(prev, next);
}

void shell_module(KernelAPI *api) {
    api->kclear_screen();
    api->kprintf("Press" YELLOW " 'help' " WHITE "for command list\n");

    while (1) {
        shell(api);
        yield();
    }
}

void proc_fs(KernelAPI *api) {
    proc_init(api);

    while (1) {
        yield();
    }
}

KernelAPI *get_scheduler_api(void) {
    return scheduler_api;
}

KernelAPI sched_api = {
    .yield = yield
}; 

void module_entry(KernelAPI *api) {
    if (!api) {
        return;
    }

    kernel_api = api;
    scheduler_api = api;

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Scheduler loaded!\n");

    api->register_scheduler(&ops);

    scheduler_init();

    // api->kernel_load_module("/bin/atom.bin");

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Loading ProcFS...\n");

    task_create_name(proc_fs, "ProcFs");
    task_create_name(shell_module, "shell");

    schedule();

    // api->kernel_load_module("/bin/shell.bin");

    // proc_init(api);

    // api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Starting Userspace!\n");
    // kernel_load_user("/bin/usr.bin", api);
    // api->kernel_load_module("/bin/usr.bin");

    // test_task(api);

    while (1) {}

    // scheduler_init();
}