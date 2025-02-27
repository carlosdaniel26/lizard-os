#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <multiboot2.h>

#define FONT_WIDTH	8
#define FONT_HEIGHT	16

void clear_framebuffer();
void setup_framebuffer(struct multiboot_tag_framebuffer_common *cfb_tag);
void draw_pixel(uint64_t x, uint64_t y, uint32_t color);
void draw_char(uint64_t x, uint64_t y, uint32_t color, char character);

#endif