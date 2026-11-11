#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <heap.h>
#include <pmm.h>
#include <kprint.h>
#include <panic.h>

static heap_block_t *heap_head;
static size_t used_bytes;
static uint8_t *heap_base = NULL;
static size_t heap_capacity = 0;
static uint64_t heap_phys = 0;

static heap_block_t *first_block = NULL;
static size_t used_bytes = 0;

static size_t align_up(size_t n) {
    return (n + HEAP_ALIGN - 1) & ~(size_t)(HEAP_ALIGN - 1);
}

void heap_init(void) {
    heap_phys = pmm_alloc_pages(HEAP_PAGES);

    if (!heap_phys) {
        panic("HEAP: Allocation failed!\n");
        return;
    }

    uint64_t virt = HHDM_BASE + heap_phys;

    heap_base = (uint8_t *)virt;

    log_kernel("HEAP Phys: %llx\n", (unsigned long long)heap_phys);
    log_kernel("HEAP Virt: %llx\n", (unsigned long long)heap_base);
    log_kernel("HEAP End: %llx\n", (unsigned long long)(heap_base + HEAP_INITIAL_SIZE));

    memset(heap_base, 0, HEAP_INITIAL_SIZE);

    heap_capacity = HEAP_INITIAL_SIZE;

    first_block = (heap_block_t *)heap_base;
    first_block->size = heap_capacity - sizeof(heap_block_t);
    first_block->free = true;
    first_block->next = NULL;

    heap_head = first_block;

    used_bytes = 0;

    log_kernel("HEAP: Initialized!\n");
}

void *kmalloc(size_t size) {
    if (size == 0 || heap_head == NULL) {
        return NULL;
    }

    size = align_up(size);

    for (heap_block_t *b = heap_head; b != NULL; b = b->next) {
        if (!b->free || b->size < size) {
            continue;
        }

        if (b->size >= size + sizeof(heap_block_t) + HEAP_ALIGN) {
            heap_block_t *rest =
                (heap_block_t *)((uint8_t *)(b + 1) + size);

            rest->size = b->size - size - sizeof(heap_block_t);
            rest->free = true;
            rest->next = b->next;

            b->next = rest;
            b->size = size;
        }

        b->free = false;
        used_bytes += b->size;
        return (void *)(b + 1);
    }

    return NULL;
}

void *kcalloc(size_t count, size_t size) {
    if (size != 0 && count > SIZE_MAX / size) {
        return NULL;
    }

    size_t total = count * size;
    void *ptr = kmalloc(total);

    if (ptr != NULL) {
        memset(ptr, 0, total);
    }

    return ptr;
}

void kfree(void *ptr) {
    if (ptr == NULL || heap_head == NULL) {
        return;
    }

    heap_block_t *b = heap_head;

    while (b != NULL && (void *)(b + 1) != ptr)
        b = b->next;

    if (b == NULL || b->free) {
        return;
    }

    b->free = true;
    used_bytes -= b->size;

    for (b = heap_head; b != NULL && b->next != NULL; ) {
        if (b->free && b->next->free) {
            b->size += sizeof(heap_block_t) + b->next->size;
            b->next = b->next->next;
        } else {
            b = b->next;
        }
    }
}

size_t heap_size(void) {
    return HEAP_SIZE;
}

size_t heap_used(void) {
    return used_bytes;
}

size_t heap_free(void) {
    return HEAP_SIZE - used_bytes;
}