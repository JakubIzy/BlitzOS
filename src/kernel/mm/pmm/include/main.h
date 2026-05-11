#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../../../limine.h"

/*
FUNCTION: BIT_SET64
DESCRIPTION: Sets bit n in a packed uint64_t bitmap (one bit per page index n).
ARGUMENTS:
    - bmap - pointer to the first element of the uint64_t word array backing the bitmap.
    - n - page index (0-based); word index is n >> 6, bit index is n & 63.
RETURN: none
*/
#define BIT_SET64(bmap, n)   ((bmap)[(n) >> 6] |=  (1ULL << ((n) & 63ULL)))

/*
FUNCTION: BIT_CLR64
DESCRIPTION: Clears bit n in a packed uint64_t bitmap.
ARGUMENTS:
    - bmap - pointer to the uint64_t word array backing the bitmap.
    - n - page index (0-based).
RETURN: none
*/
#define BIT_CLR64(bmap, n)   ((bmap)[(n) >> 6] &= ~(1ULL << ((n) & 63ULL)))

/*
FUNCTION: BIT_TEST64
DESCRIPTION: macro to get value of bit n in the bitmap (0 or 1).
ARGUMENTS:
    - bmap - pointer to the uint64_t word array backing the bitmap.
    - n - page index (0-based).
RETURN: 1ULL if the bit is set, 0ULL if clear.
*/
#define BIT_TEST64(bmap, n)  (((bmap)[(n) >> 6] >> ((n) & 63ULL)) & 1ULL)

struct PMM {
    uint64_t *bitmap;
    uint64_t pages_count;
    uint64_t phys_limit;
};

/*
FUNCTION: pmm_init
DESCRIPTION:
Builds the physical page allocator from
the Limine memory map:
- allocates a packed bitmap via bootmm,
- marks all frames reserved,
- then marks Limine USABLE regions free,
- and reserves page 0 and the bitmap storage.
Bit 1 means used/reserved, bit 0 means free.
ARGUMENTS:
    - phys_limit - exclusive top of PA space to cover (bytes)
    - memmap - Limine memmap response pointer; non-NULL.
RETURN: true - success; false - memmap NULL | allocation fail
*/
bool pmm_init(uint64_t phys_limit, struct limine_memmap_response *memmap);

/*
FUNCTION: pmm_reserve_range
DESCRIPTION: Marks every page overlapping
[base_phys, base_phys + length) as used in the PMM bitmap
(no allocation of backing memory).
ARGUMENTS:
    - base_phys - physical start address of the range (bytes).
    - length - length of the range in bytes;
    if base_phys + length overflows, the range is ignored.
RETURN: none
*/
void pmm_reserve_range(uint64_t base_phys, uint64_t length);

/*
FUNCTION: pmm_alloc_page
DESCRIPTION: Allocates a single 4 KiB physical page
with the lowest index and which has clear bit,
ARGUMENTS: none
RETURN: HHDM VPtr to the page base on success;
        NULL - PMM uninitialized | Out-of-memory.
*/
void* pmm_alloc_page();

/*
FUNCTION: pmm_free_page
DESCRIPTION: Frees one page previously returned
by pmm_alloc_page (or pmm_alloc_pages for a single page).
Validates that the pointer is HHDM-mapped, page-aligned,
and currently marked used; otherwise does nothing.
ARGUMENTS:
    - ptr - HHDM VA of the page base to free; NULL is ignored.
RETURN: none
*/
void pmm_free_page(void *ptr);

/*
FUNCTION: pmm_is_page_used
DESCRIPTION: Queries whether the frame containing
the given VA is marked used in the PMM bitmap.
ARGUMENTS:
    - ptr - HHDM VA; must be page-aligned for
    a meaningful result; NULL yields false.
RETURN: true - page used;
false - invalid | out of range | unaligned | page unused.
*/
bool pmm_is_page_used(void *ptr);

/*
FUNCTION: pmm_alloc_pages
DESCRIPTION: Allocates a contiguous run of count 4 KiB pages
using a first-fit scan of the bitmap; sets all bits
in the run and returns the HHDM VPtr to the first page.
ARGUMENTS:
    - count - number of contiguous pages required;
    must be greater than zero.
RETURN: HHDM VPtr to the first page on success;
NULL if count is zero, PMM is uninitialized,
or no sufficiently long free run exists.
*/
void *pmm_alloc_pages(uint64_t count);

/*
FUNCTION: pmm_free_pages
DESCRIPTION: Clears the used bits for count contiguous pages
starting at ptr (HHDM base of the first page).
Silently skips pages already marked free within the range.
ARGUMENTS:
    - ptr - HHDM VA of the first page base; page-aligned.
    - count - number of contiguous pages to free;
    zero is a no-op.
RETURN: none
*/
void pmm_free_pages(void *ptr, uint64_t count);

/*
FUNCTION: pmm_phys_to_virt
DESCRIPTION: Maps a PA to the canonical kernel VA using
the Limine HHDM offset (phys + hhdm_offset).
ARGUMENTS:
    - phys - PA in bytes.
RETURN: void pointer equal to phys + hhdm_offset;
no range check is performed.
*/
void* pmm_phys_to_virt(uint64_t phys);

/*
FUNCTION: pmm_virt_to_phys
DESCRIPTION: Maps an HHDM kernel VA back to PA.
Pointers below the HHDM base are rejected.
ARGUMENTS:
    - virt - kernel VPtr to convert.
    - ok - if non-NULL, set to true on success
    and false on failure (virt not in HHDM range).
RETURN: PA on success; 0 on failure
(check *ok when ok is non-NULL to distinguish
from a valid physical 0 if ever exposed).
*/
uint64_t pmm_virt_to_phys(void *virt, bool *ok);

/*
FUNCTION: pmm_total_pages_count
DESCRIPTION: Returns the number of physical pages
covered by the current PMM bitmap
(derived from phys_limit at init).
ARGUMENTS: none
RETURN: page count; 0 if the PMM was never successfully
initialized with a non-zero span.
*/
uint64_t pmm_total_pages_count();

/*
FUNCTION: pmm_free_pages_count
DESCRIPTION: Counts pages whose bitmap bit is currently clear.
ARGUMENTS: none
RETURN: number of free pages; 0 if uninitialized or all used.
*/
uint64_t pmm_free_pages_count();

/*
FUNCTION: pmm_used_pages_count
DESCRIPTION: Counts pages whose bitmap bit is currently set.
ARGUMENTS: none
RETURN: number of used pages; 0 if uninitialized or all free.
*/
uint64_t pmm_used_pages_count();
