#include "include/main.h"
#include "../../limine.h"
#include <stdint.h>

static struct framebuffer fb;

void fb_init(struct limine_framebuffer *limine_fb) {
    fb.address = limine_fb->address;

    fb.width = limine_fb->width;
    fb.height = limine_fb->height;
    fb.pitch = limine_fb->pitch;
    fb.bpp = limine_fb->bpp;

    fb.memory_model = limine_fb->memory_model;

    fb.red_mask_size = limine_fb->red_mask_size;
    fb.red_mask_shift = limine_fb->red_mask_shift;
    fb.green_mask_size = limine_fb->green_mask_size;
    fb.green_mask_shift = limine_fb->green_mask_shift;
    fb.blue_mask_size = limine_fb->blue_mask_size;
    fb.blue_mask_shift = limine_fb->blue_mask_shift;

    fb.edid = limine_fb->edid;
    fb.edid_size = limine_fb->edid_size;

    fb.mode_count = limine_fb->mode_count;
    fb.modes = limine_fb->modes;
}

const struct framebuffer *fb_get(void) {
    return &fb;
}

int fb_putpixel(uint64_t x, uint64_t y, uint32_t color) {
    if (fb.address == 0 || fb.bpp != 32) return 1;
    if (
        x >= fb.width ||
        y >= fb.height
    ) {
        return 2;
    }

    ((volatile uint32_t *)fb.address)[y * (fb.pitch / 4) + x] = color;
    return 0;
}

void fb_draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color) {
    for (uint64_t yy = y; yy<y+height; yy++) {
        for (uint64_t xx = x; xx<x+width; xx++) {
            fb_putpixel(xx, yy, color);
        }
    }
}