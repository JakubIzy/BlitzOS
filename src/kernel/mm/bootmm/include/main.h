#pragma once
#include <stdint.h>

/*
FUNCTION: ALIGN_UP (macro)
DESCRIPTION: Aligns integer value x upwards to boundary a. a must be a power of two.
ARGUMENTS:
    - x - value to align (any integer expression).
    - a - alignment; must divide the address/size space cleanly (power of two).
RETURN: smallest value >= x that is divisible by a.
*/
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

/* Limine HHDM offset: physical + hhdm_offset => canonical kernel virtual pointer. Set before bootmm_alloc. */
extern uint64_t hhdm_offset;
/* Linker symbol: VA end of kernel image (.bss inclusive). Used with executable address response for physical placement. */
extern char _kernel_end[];

typedef struct {
    uint64_t current;
    uint64_t limit;
} bump_allocator_t;

/*
FUNCTION: bootmm_phys_to_virt
DESCRIPTION: Maps a PA to a kernel-accessible VA using the HHDM offset from Limine.
ARGUMENTS:
    - phys - PA within RAM covered by HHDM (bytes).
RETURN: pointer equivalent to phys + hhdm_offset; must not be called before hhdm_offset is set.
*/
void *bootmm_phys_to_virt(uint64_t phys);

/*
FUNCTION: bootmm_init
DESCRIPTION: Configures the early bump allocator to hand out backing store from one contiguous physical range.
ARGUMENTS:
    - start_phys - PA where the allocator starts (typically page-aligned, just past kernel/end).
    - size - maximum number of bytes that may be bumped from start_phys onward.
RETURN: none
*/
void bootmm_init(uint64_t start_phys, uint64_t size);

/*
FUNCTION: bootmm_memset
DESCRIPTION: Fills memory with a byte value—freestanding equivalent for zeroing allocations without libc.
ARGUMENTS:
    - dest - destination address (typically from bootmm_alloc).
    - value - byte pattern to write (converted to unsigned char).
    - count - number of bytes to fill.
RETURN: dest (same as libc memset).
*/
void *bootmm_memset(void *dest, int value, uint64_t count);

/*
FUNCTION: bootmm_alloc
DESCRIPTION: Bump-allocates aligned bytes from the configured range.
ARGUMENTS:
    - size - number of bytes to reserve.
    - align - alignment requirement; must be a non-zero power of two (bytes).
RETURN: HHDM virtual pointer on success; NULL if align is invalid, out of RAM, or addition would overflow.
*/
void *bootmm_alloc(uint64_t size, uint64_t align);

/*
FUNCTION: bootmm_allocz
DESCRIPTION: Like bootmm_alloc, then zero-fills the returned region with bootmm_memset.
ARGUMENTS:
    - size - number of bytes to reserve.
    - align - alignment requirement; must be a non-zero power of two (bytes).
RETURN: zeroed HHDM virtual pointer on success; NULL on allocation failure.
*/
void *bootmm_allocz(uint64_t size, uint64_t align);
