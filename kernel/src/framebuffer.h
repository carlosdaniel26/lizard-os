#pragma once

#include <stddef.h>
#include <types.h>
#include <string.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

void clear_framebuffer();
void setup_framebuffer(u64 w, u64 h, u32 *fb, u32 pth);
void draw_pixel(u64 x, u64 y, u32 color);
void draw_char(u64 x_index, u64 y_index, u32 color, char character);
void scroll_framebuffer(u32 pixels);

extern u32 *framebuffer;
extern u64 height;
extern u64 width;
extern u32 pitch;

extern u32 framebuffer_length;

