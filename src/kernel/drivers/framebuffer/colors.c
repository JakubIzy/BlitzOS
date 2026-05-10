#include "include/colors.h"
#include <stdint.h>
#include "include/main.h"

static uint32_t scale_8bit_to_nbit(uint8_t value, uint8_t nbits) {
    if (nbits == 0) return 0;
    if (nbits >= 8) return (uint32_t)value << (nbits - 8);
    uint32_t max_n = (1u << nbits) - 1u;
    return ((uint32_t)value * max_n + 127u) / 255u; // rounded scaling
}

uint32_t fb_color(
    uint8_t red,
    uint8_t green,
    uint8_t blue
) {
    const struct framebuffer *fb = fb_get();

    uint32_t r = scale_8bit_to_nbit(red, fb->red_mask_size);
    uint32_t g = scale_8bit_to_nbit(green, fb->green_mask_size);
    uint32_t b = scale_8bit_to_nbit(blue, fb->blue_mask_size);

    return (r << fb->red_mask_shift) |
           (g << fb->green_mask_shift) |
           (b << fb->blue_mask_shift);
}