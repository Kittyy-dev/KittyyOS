#pragma once

#include <stdint.h>

#define X86_RFLAGS_IF (1UL << 9)
#define SPINLOCK_INIT { .locked = 0 }

typedef struct {
    uint32_t locked;
} spinlock_t;

static inline void spin_lock(spinlock_t *lock) {
    while (__atomic_exchange_n(&lock->locked, 1, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&lock->locked, __ATOMIC_RELAXED)) {
            __asm__ volatile("pause");
        }
    }
}

static inline void spin_unlock(spinlock_t *lock) {
    __atomic_store_n(&lock->locked, 0, __ATOMIC_RELEASE);
}

typedef unsigned long irq_flags_t;

static inline irq_flags_t local_irq_save(void) {
    irq_flags_t flags;

    __asm__ volatile ("pushfq\n\t" "popq %0\n\t" "cli" : "=r"(flags) :: "memory");

    return flags;
}

static inline void local_irq_restore(irq_flags_t flags) {
    if (flags & X86_RFLAGS_IF) {
        __asm__ volatile ("sti" ::: "memory");
    } else {
        __asm__ volatile ("cli" ::: "memory");
    }
}

#define spin_lock_irqsave(lock, flags) \
    do {                               \
        (flags) = local_irq_save();    \
        spin_lock(lock);               \
    } while (0)


#define spin_unlock_irqrestore(lock, flags) \
    do {                                    \
        spin_unlock(lock);                  \
        local_irq_restore(flags);           \
    } while (0)
