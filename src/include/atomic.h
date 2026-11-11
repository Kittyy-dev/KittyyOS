
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t value;
} atomic64_t;

static inline uint64_t atomic_load64(const atomic64_t *a) {
    return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST);
}

static inline void atomic_store64(atomic64_t *a, uint64_t value) {
    __atomic_store_n(&a->value, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic_fetch_add64(atomic64_t *a, uint64_t value) {
    return __atomic_fetch_add(&a->value, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic_fetch_sub64(atomic64_t *a, uint64_t value) {
    return __atomic_fetch_sub(&a->value, value, __ATOMIC_SEQ_CST);
}

static inline bool atomic_compare_exchange64(atomic64_t *a, uint64_t *expected, uint64_t desired) {
    return __atomic_compare_exchange_n(&a->value, expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}