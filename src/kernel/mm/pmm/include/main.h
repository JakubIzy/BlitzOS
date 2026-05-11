#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../../../limine.h"

#define BIT_SET64(bmap, n)   ((bmap)[(n) >> 6] |=  (1ULL << ((n) & 63ULL)))
#define BIT_CLR64(bmap, n)   ((bmap)[(n) >> 6] &= ~(1ULL << ((n) & 63ULL)))
#define BIT_TEST64(bmap, n)  (((bmap)[(n) >> 6] >> ((n) & 63ULL)) & 1ULL)

struct PMM {
    uint64_t *bitmap;
    uint64_t pages_count;
    uint64_t phys_limit;
};

bool pmm_init(uint64_t phys_limit, struct limine_memmap_response *memmap);

void pmm_reserve_range(uint64_t base_phys, uint64_t length);

void* pmm_alloc_page();

void pmm_free_page(void *ptr);

bool pmm_is_page_used(void *ptr);

void *pmm_alloc_pages(uint64_t count);

void pmm_free_pages(void *ptr, uint64_t count);

void* pmm_phys_to_virt(uint64_t phys);
/* On success sets *ok true and returns physical address; on failure *ok false (if ok non-NULL) and returns 0. */
uint64_t pmm_virt_to_phys(void *virt, bool *ok);

uint64_t pmm_total_pages_count();
uint64_t pmm_free_pages_count();
uint64_t pmm_used_pages_count();