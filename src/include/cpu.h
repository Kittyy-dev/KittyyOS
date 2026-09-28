#include <stdint.h>
#include <task.h>

#define MAX_CPUS 256

struct cpu {
    uint32_t id;
    uint32_t apic_id;

    uint64_t kernel_stack;

    struct task* current_task;
};

void cpu_vendor(char* out);
void cpu_brand(char* out);
uint32_t cpu_id(void);
void cpu_init_bsp(void);
void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t* ecx, uint32_t* edx);
uint32_t cpuid_max_leaf(void);
uint32_t cpu_stepping(void);
uint32_t cpu_family(void);
uint32_t cpu_model(void);
uint32_t cpu_apic_id(void);
uint32_t cpuid_max_extended_leaf(void);