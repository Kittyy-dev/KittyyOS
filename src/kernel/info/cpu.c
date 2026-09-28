#include <stdint.h>
#include <cpu.h>
#include <string.h>

#define MSR_GS_BASE 0xC0000101

static struct cpu cpus[MAX_CPUS];
static uint32_t cpu_count = 0;

void cpu_vendor(char* out) { 
    uint32_t eax, ebx, ecx, edx;

    __asm__ volatile("mov $0, %%eax\n\t" "cpuid\n\t" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));

    *(uint32_t*)(out + 0) = ebx;
    *(uint32_t*)(out + 4) = edx;
    *(uint32_t*)(out + 8) = ecx;
    out[12] = 0;
}

void cpu_brand(char* out) {
    uint32_t max;

    __asm__ volatile(
        "cpuid"
        : "=a"(max)
        : "a"(0x80000000)
        : "ebx", "ecx", "edx"
    );

    if (max < 0x80000004) {
        out[0] = 0;
        return;
    }

    uint32_t regs[4];

    for (int i = 0; i < 3; i++) {
        __asm__ volatile(
            "cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(0x80000002 + i)
        );

        memcpy(out + i * 16 + 0, &regs[0], 4);
        memcpy(out + i * 16 + 4, &regs[1], 4);
        memcpy(out + i * 16 + 8, &regs[2], 4);
        memcpy(out + i * 16 + 12, &regs[3], 4);
    }

    out[48] = 0;
}

void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t* ecx, uint32_t* edx) {
    uint32_t a, b, c, d;

    __asm__ volatile ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(subleaf));

    if (eax) * eax = a;
    if (ebx) * ebx = b;
    if (ecx) * ecx = c;
    if (edx) * edx = d; 
}

uint32_t cpuid_max_leaf(void) {
    uint32_t eax, ebx, ecx, edx;

    cpuid(0, 0, &eax, &ebx, &ecx, &edx);

    return eax;
}

uint32_t cpu_stepping(void) {
    uint32_t eax, ebx, ecx, edx;

    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    return eax & 0xF;
}

uint32_t cpu_family(void) {
    uint32_t eax, ebx, ecx, edx;

    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    uint32_t family = (eax >> 8) & 0xF;
    uint32_t extended_family = (eax >> 20) & 0xFF;

    if (family == 0xF) {
        family += extended_family;
    }

    return family;
}

uint32_t cpu_model(void) {
    uint32_t eax, ebx, ecx, edx;

    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    uint32_t family = (eax >> 8) & 0xF;
    uint32_t model = (eax >> 4) & 0xF;
    uint32_t extended_model = (eax >> 16) & 0xF;

    if (family == 0x6 || family == 0xF) {
        model |= extended_model << 4;
    }

    return model;
}

uint32_t cpu_apic_id(void) {
    uint32_t eax, ebx, ecx, edx;
 
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    return (ebx >> 24) & 0xFF;
}

uint32_t cpuid_max_extended_leaf(void) {
    uint32_t eax, ebx, ecx, edx;

    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);

    return eax;
}