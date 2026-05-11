#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"
#include "drivers/framebuffer/include/main.h"
#include "drivers/framebuffer/include/colors.h"
#include "mm/bootmm/include/main.h"
#include "mm/pmm/include/main.h"

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

extern char _kernel_end[];

static uint64_t g_kernel_phys_base;
static uint64_t g_kernel_phys_end_exclusive;
// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request exec_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = dest;
    const uint8_t *restrict psrc = src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    if ((uintptr_t)src > (uintptr_t)dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if ((uintptr_t)src < (uintptr_t)dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void init_early_allocator(void) {
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_executable_address_response *exec = exec_address_request.response;
    struct limine_memmap_response *mmap = memmap_request.response;

    if (hhdm == NULL || exec == NULL || mmap == NULL) {
        hcf();
    }

    hhdm_offset = hhdm->offset;

    uint64_t vbase = exec->virtual_base;
    uint64_t pbase = exec->physical_base;
    uintptr_t kern_end_virt = ALIGN_UP((uintptr_t)_kernel_end, 4096);

    /* Image span in VA equals span in PA for the contiguous Limine-loaded blob. */
    if (kern_end_virt < vbase) {
        hcf();
    }
    uint64_t kernel_end_phys = ALIGN_UP(pbase + (kern_end_virt - vbase), 4096);

    g_kernel_phys_base = pbase;
    g_kernel_phys_end_exclusive = kernel_end_phys;

    for (uint64_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *entry = mmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t base = entry->base;
        uint64_t end = base + entry->length;

        if (end < base) {
            continue;
        }

        if (kernel_end_phys >= base &&
            kernel_end_phys < end) {
            bootmm_init(kernel_end_phys, end - kernel_end_phys);
            return;
        }
    }

    hcf();
}

// Kernel's entry point
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Print a nice pattern to screen as an example.
    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    // volatile uint32_t *fb_ptr = framebuffer->address;
    // for (size_t y = 0; y < framebuffer->height; y++) {
    //     for (size_t x = 0; x < framebuffer->width; x++) {
    //         uint32_t nX = x * 255 / framebuffer->width;
    //         uint32_t nY = y * 255 / framebuffer->height;
    //         fb_ptr[y * (framebuffer->pitch / 4) + x] = (nY << 8) | nX;
    //     }
    // }

    fb_init(framebuffer);
    fb_draw_rect(100, 100, 200, 150, fb_color(255, 255, 255));

    // Init bootmm
    init_early_allocator();

    // Init pmm
    struct limine_memmap_response *memmap = memmap_request.response;

    if (memmap == NULL) {
        hcf();
    }

    uint64_t phys_top = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        uint64_t end = e->base + e->length;

        if (end < e->base) {
            /* overflow on base+length — skip or handle */
            continue;
        }
        if (end > phys_top) {
            phys_top = end;
        }
    }
    bool pmm_success = pmm_init(ALIGN_UP(phys_top, 4096), memmap);
    if (!pmm_success) {
        hcf();
    }

    if (g_kernel_phys_end_exclusive > g_kernel_phys_base) {
        pmm_reserve_range(g_kernel_phys_base,
                          g_kernel_phys_end_exclusive - g_kernel_phys_base);
    }

    uint64_t boot_lo = bootmm_region_start_phys();
    uint64_t boot_hi = bootmm_region_limit_phys();
    if (boot_hi > boot_lo) {
        pmm_reserve_range(boot_lo, boot_hi - boot_lo);
    }

    // We're done, just hang...
    hcf();
}