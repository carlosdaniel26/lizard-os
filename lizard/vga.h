#pragma once

#include <types.h>

#define VGA_WIDTH 1024
#define VGA_HEIGHT 768

enum vga_color
{
    VGA_COLOR_BLACK = 0x000000,         /* Black: RGB(0, 0, 0)*/
    VGA_COLOR_BLUE = 0x0000FF,          /* Blue: RGB(0, 0, 255)*/
    VGA_COLOR_GREEN = 0x00FF00,         /* Green: RGB(0, 255, 0)*/
    VGA_COLOR_CYAN = 0x00FFFF,          /* Cyan: RGB(0, 255, 255)*/
    VGA_COLOR_RED = 0xFF0000,           /* Red: RGB(255, 0, 0)*/
    VGA_COLOR_MAGENTA = 0xFF00FF,       /* Magenta: RGB(255, 0, 255)*/
    VGA_COLOR_BROWN = 0xA52A2A,         /* Brown: RGB(165, 42, 42)*/
    VGA_COLOR_LIGHT_GREY = 0xD3D3D3,    /* Light Grey: RGB(211, 211, 211)*/
    VGA_COLOR_DARK_GREY = 0xA9A9A9,     /* Dark Grey: RGB(169, 169, 169)*/
    VGA_COLOR_LIGHT_BLUE = 0xADD8E6,    /* Light Blue: RGB(173, 216, 230)*/
    VGA_COLOR_LIGHT_GREEN = 0x90EE90,   /* Light Green: RGB(144, 238, 144)*/
    VGA_COLOR_LIGHT_CYAN = 0xE0FFFF,    /* Light Cyan: RGB(224, 255, 255)*/
    VGA_COLOR_LIGHT_RED = 0xFFB6C1,     /* Light Red: RGB(255, 182, 193)*/
    VGA_COLOR_LIGHT_MAGENTA = 0xFF77FF, /* Light Magenta: RGB(255, 119, 255)*/
    VGA_COLOR_LIGHT_BROWN = 0xF4A460,   /* Light Brown: RGB(244, 164, 96)*/
    VGA_COLOR_YELLOW = 0xFFFF00,        /* Yellow: RGB(255, 255, 0) */
    VGA_COLOR_WHITE = 0xFFFFFF          /* White: RGB(255, 255, 255)*/
};
