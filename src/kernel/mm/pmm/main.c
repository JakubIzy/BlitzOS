#include "include/main.h"
#include "../bootmm/include/main.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../limine.h"


static struct PMM PMM;

static void pmm_set_page_state(uint64_t page, bool used) {
    if (page >= PMM.pages_count || PMM.bitmap == NULL) {
        return;
    }

    if (used) {
        BIT_SET64(PMM.bitmap, page);
    } else {
        BIT_CLR64(PMM.bitmap, page);
    }
}

static void pmm_set_range_state(uint64_t base, uint64_t length, bool used) {
    uint64_t end = base + length;
    if (end < base) {
        return;
    }

    uint64_t start_page = base / 4096ULL;
    uint64_t end_page = ALIGN_UP(end, 4096ULL) / 4096ULL;

    for (uint64_t page = start_page; page < end_page && page < PMM.pages_count; page++) {
        pmm_set_page_state(page, used);
    }
}

void pmm_reserve_range(uint64_t base_phys, uint64_t length) {
    pmm_set_range_state(base_phys, length, true);
}

bool pmm_init(uint64_t phys_limit, struct limine_memmap_response *memmap) {
    if (memmap == NULL) {
        return false;
    }

    uint64_t pages = ALIGN_UP(phys_limit, 4096ULL) / 4096ULL;
    uint64_t bitmap_words = (pages + 63) / 64;
    uint64_t bitmap_bytes = bitmap_words * sizeof(uint64_t);
    uint64_t *bitmap =
        bitmap_bytes ? (uint64_t *)bootmm_allocz(bitmap_bytes, sizeof(uint64_t)) : NULL;
    if (bitmap_bytes != 0 && bitmap == NULL) {
        return false;
    }

    PMM.phys_limit = phys_limit;
    PMM.bitmap = bitmap;
    PMM.pages_count = pages;
    if (bitmap_bytes != 0) {
        bootmm_memset(PMM.bitmap, 0xff, bitmap_bytes);
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uint64_t base = entry->base;
        uint64_t len = entry->length;

        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                pmm_set_range_state(base, len, false);
                break;
            default:
                break;
        }
    }

    /* Keep page 0 reserved (NULL-guard page) and reserve PMM metadata storage. */
    pmm_set_range_state(0, 4096ULL, true);
    if (bitmap_bytes != 0) {
        uint64_t bitmap_phys = (uint64_t)PMM.bitmap - hhdm_offset;
        pmm_set_range_state(bitmap_phys, bitmap_bytes, true);
    }
    return true;
}

void* pmm_alloc_page() {
    if (PMM.bitmap == NULL || PMM.pages_count == 0) {
        return NULL;
    }

    for (uint64_t i = 0; i < PMM.pages_count; i++) {
        if (BIT_TEST64(PMM.bitmap, i) == 0) {
            uint64_t phys = 4096ULL * i;
            BIT_SET64(PMM.bitmap, i);
            return bootmm_phys_to_virt(phys);
        }
    }
    return NULL;
}

void pmm_free_page(void *ptr) {
    if (ptr == NULL || PMM.bitmap == NULL) {
        return;
    }
    uint64_t virt = (uint64_t)ptr;
    if (virt < hhdm_offset) {
        return;
    }

    uint64_t phys = virt - hhdm_offset;

    if ((phys & (4096ULL - 1ULL)) != 0) {
        return;
    }

    uint64_t page = phys / 4096ULL;
    if (page >= PMM.pages_count) {
        return;
    }

    if (BIT_TEST64(PMM.bitmap, page) == 0) {
        return;
    }

    BIT_CLR64(PMM.bitmap, page);
}

bool pmm_is_page_used(void *ptr) {
    if (ptr == NULL || PMM.bitmap == NULL) {
        return false;
    }
    uint64_t virt = (uint64_t)ptr;
    if (virt < hhdm_offset) {
        return false;
    }

    uint64_t phys = virt - hhdm_offset;

    if ((phys & (4096ULL - 1ULL)) != 0) {
        return false;
    }

    uint64_t page = phys / 4096ULL;
    if (page >= PMM.pages_count) {
        return false;
    }
    return BIT_TEST64(PMM.bitmap, page) != 0;
}

void *pmm_alloc_pages(uint64_t count) {
    if (count == 0 || PMM.bitmap == NULL || PMM.pages_count == 0) {
        return NULL;
    }

    uint64_t run_start = 0;
    uint64_t run_len = 0;

    for (uint64_t page = 0; page < PMM.pages_count; page++) {
        if (BIT_TEST64(PMM.bitmap, page) == 0) {
            if (run_len == 0) {
                run_start = page;
            }
            run_len++;

            if (run_len == count) {
                if (run_start > UINT64_MAX - count) {
                    return NULL;
                }
                for (uint64_t p = run_start; p < run_start + count; p++) {
                    BIT_SET64(PMM.bitmap, p);
                }

                uint64_t phys = run_start * 4096ULL;
                return bootmm_phys_to_virt(phys);
            }
        } else {
            run_len = 0;
        }
    }

    return NULL;
}

void pmm_free_pages(void *ptr, uint64_t count) {
    if (ptr == NULL || count == 0 || PMM.bitmap == NULL) {
        return;
    }

    uint64_t virt = (uint64_t)ptr;
    if (virt < hhdm_offset) {
        return;
    }

    uint64_t phys = virt - hhdm_offset;
    if ((phys & (4096ULL - 1ULL)) != 0) {
        return;
    }

    uint64_t start_page = phys / 4096ULL;

    if (start_page >= PMM.pages_count) {
        return;
    }
    if (count > PMM.pages_count - start_page) {
        return;
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t page = start_page + i;

        if (BIT_TEST64(PMM.bitmap, page) == 0) {
            continue;
        }

        BIT_CLR64(PMM.bitmap, page);
    }
}

void *pmm_phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

uint64_t pmm_virt_to_phys(void *virt, bool *ok) {
    if (ok != NULL) {
        *ok = false;
    }

    if ((uint64_t)virt < hhdm_offset) {
        return 0;
    }

    if (ok != NULL) {
        *ok = true;
    }
    return (uint64_t)virt - hhdm_offset;
}

uint64_t pmm_total_pages_count() {
    return PMM.pages_count;
}
uint64_t pmm_used_pages_count() {
    uint64_t used = 0;
    for (uint64_t page = 0; page < PMM.pages_count; page++) {
        if (BIT_TEST64(PMM.bitmap, page) == 1) {
            used++;
        }
    }
    return used;
}
uint64_t pmm_free_pages_count() {
    uint64_t free = 0;
    for (uint64_t page = 0; page < PMM.pages_count; page++) {
        if (BIT_TEST64(PMM.bitmap, page) == 0) {
            free++;
        }
    }
    return free;
}