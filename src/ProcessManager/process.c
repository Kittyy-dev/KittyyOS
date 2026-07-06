#include <process.h>
#include <vga.h>
#include <heap.h>
#include <colors.h>

Process process_table[MAX_PROCESSES];
Process* current_process = NULL;
static uint32_t next_pid = 1;

void process_exit(void) {
    while (1);
}

void init_process_manager(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_UNUSED;
        process_table[i].pid = 0;
    }
    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "Process Manager Initialized!\n");
}

int create_process(void (*entry)(void)) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            process_table[i].pid = next_pid++;
            process_table[i].state = PROC_READY;
            process_table[i].stack = kmalloc(4096);
            uint64_t* sp = (uint64_t*)((uint8_t*)process_table[i].stack + 4096);

            sp -= 1;
            sp[0] = (uint64_t)entry; 
            sp[1] = (uint64_t)process_exit;             

            process_table[i].regs[0] = (uint64_t)sp; 

            vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "Process Created!\n");

            return process_table[i].pid;
        }
    }
    return -1;
}

void schedule(void) {
    int next = -1;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_READY) {
            next = i;
            break;
        }
    }

    if (next >= 0) {
        Process* new = &process_table[next];
        Process* old = current_process;

        if (old) old->state = PROC_READY;
        new->state = PROC_RUNNING;
        current_process = new;

        context_switch(old, new);
    }
}
