#include "include/main.h"
#include <stddef.h>
#include <stdint.h>

uint64_t hhdm_offset;

static bump_allocator_t early_alloc;
static uint64_t region_start_phys;
static uint64_t region_limit_phys;

void *bootmm_phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

void bootmm_init(uint64_t start_phys, uint64_t size) {
    region_start_phys = start_phys;
    region_limit_phys = start_phys + size;
    early_alloc.current = start_phys;
    early_alloc.limit = region_limit_phys;
}

uint64_t bootmm_region_start_phys(void) {
    return region_start_phys;
}

uint64_t bootmm_region_limit_phys(void) {
    return region_limit_phys;
}

void *bootmm_memset(void *dest, int value, uint64_t count) {
    uint8_t *ptr = dest;

    for (uint64_t i = 0; i < count; i++) {
        ptr[i] = (uint8_t)value;
    }

    return dest;
}

void *bootmm_alloc(uint64_t size, uint64_t align) {
    if (align == 0 || ((align & (align - 1)) != 0)) {
        return NULL;
    }

    uint64_t current = ALIGN_UP(early_alloc.current, align);
    uint64_t next = current + size;

    if (next > early_alloc.limit || next < current) {
        return NULL;
    }

    early_alloc.current = next;
    return bootmm_phys_to_virt(current);
}

void *bootmm_allocz(uint64_t size, uint64_t align) {
    void *ptr = bootmm_alloc(size, align);

    if (!ptr) {
        return NULL;
    }
    bootmm_memset(ptr, 0, size);
    return ptr;
}
