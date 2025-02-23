#include <multiboot2.h>

void clear_framebuffer();
void setup_framebuffer(struct multiboot_tag_framebuffer_common *cfb_tag);
void draw_pixel(uint64_t x, uint64_t y, uint32_t color);
void draw_char(uint64_t x, uint64_t y, uint32_t color, char character);
void print_hello(uint64_t start_x, uint64_t start_y);