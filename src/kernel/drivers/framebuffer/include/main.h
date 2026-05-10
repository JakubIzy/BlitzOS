#pragma once
#include <stdint.h>
#include "../../../limine.h"


// Framebuffer struct based on limine_framebuffer, so we can store it internally
struct framebuffer {
    void *address;

    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp; // Bits per pixel

    uint8_t memory_model;

    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;

    uint64_t edid_size;
    void *edid;

    uint64_t mode_count;
    struct limine_video_mode **modes;
};


/*
FUNCTION: fb_get
DESCRIPTION: returns pointer to framebuffer
ARGUMENTS: none
RETURN: Pointer to framebuffer
*/
const struct framebuffer *fb_get(void);

/*
FUNCTION: fb_init
DESCRIPTION: initializes internal framebuffer descriptor
ARGUMENTS:
    - struct limine_framebuffer *limine_fb - pointer to framebuffer provided by Limine
RETURN: none
*/
void fb_init(struct limine_framebuffer *limine_fb);

/*
FUNCTION: fb_putpixel
DESCRIPTION: puts pixel on the screen
ARGUMENTS:
    - uint64_t x - x coordinate of the pixel
    - uint64_t y - y coordinate of the pixel
    - uint64_t color - color in machine-compatible format (provided by fb_color)
RETURN: int
*/
int fb_putpixel(uint64_t x, uint64_t y, uint32_t color);

/*
FUNCTION: fb_draw_rect
DESCRIPTION: draw rectangle on the screen
NOTE: Will likely to be moved to separate file when other geeometry-drawing functions will be added
ARGUMENTS:
    - uint64_t x - x coordinate of top left corner of rectangle
    - uint64_t y - y coordinate of top left corner of rectangle
    - uint64_t width - width of the rectangle
    - uint64_t height - height of the rectangle
    - uint64_t color - color of the rectangle in machine-compatible format (provided by fb_color)
RETURN: none
*/
void fb_draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t color);