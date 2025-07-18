#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

void clear_framebuffer();
void setup_framebuffer(uint64_t w, uint64_t h, uint32_t *fb, uint32_t pth);
void draw_pixel(uint64_t x, uint64_t y, uint32_t color);
void draw_char(uint64_t x_index, uint64_t y_index, uint32_t color, char character);
void scroll_framebuffer(uint32_t pixels);

extern uint32_t *framebuffer;
extern uint64_t height;
extern uint64_t width;
extern uint32_t pitch;

extern uint32_t framebuffer_length;

#endif
