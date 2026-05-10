#pragma once
#include <stdint.h>
#include "main.h"


/*
FUNCTION: fb_color
DESCRIPTION: converts red, green and blue color parameters, so the machine understands them.
ARGUMENTS:
    - uint8_t red - 8-byte number representing red color
    - uint8_t green - 8-byte number representing green color
    - uint8_t blue - 8-byte number representing blue color
RETURN: 32-byte correctly aligned number representing RGB color in format supported by machine
*/
uint32_t fb_color(
    uint8_t red,
    uint8_t green,
    uint8_t blue
);