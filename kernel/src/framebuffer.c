#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <framebuffer.h>

uint32_t *framebuffer;
uint32_t height;
uint32_t width;

uint32_t terminal_background_color = 0xFFFFFF;

void clear_framebuffer()
{
	for (uint64_t i = 0; i < width * height; i++)
	{
		framebuffer[i] = terminal_background_color;
	}
}

void setup_framebuffer(uint32_t w, uint32_t h, uint32_t *fb)
{
	width = w;
	height = h;
	framebuffer = fb;
}

void draw_pixel(uint64_t x, uint64_t y, uint32_t color)
{
	framebuffer[(y * width) + x] = color;
}

void draw_char(uint64_t x_index, uint64_t y_index, uint32_t color, char character)
{
	uint64_t first_byte_idx = character * FONT_HEIGHT;
	uint32_t bg_color = terminal_background_color;
	for (size_t y = 0; y < FONT_HEIGHT; y++) {
		uint8_t row_data = font[first_byte_idx + y];
		for (size_t x = 0; x < FONT_WIDTH; x++) {
			uint32_t pixel_color = (row_data >> (7 - x)) & 1 ? color : bg_color;
			draw_pixel(x_index + x, y_index + y, pixel_color);
		}
	}
}

void scroll_framebuffer(uint32_t pixels)
{
    if (pixels == 0 || pixels >= height)
        return;

    uint32_t* fb_ptr = (uint32_t*)framebuffer;
    uint32_t scroll_offset = pixels * width;
    uint32_t move_pixels = (height - pixels) * width;

    memmove(fb_ptr, fb_ptr + scroll_offset, move_pixels * sizeof(uint32_t));

    uint32_t* clear_start = fb_ptr + move_pixels;
    for (uint32_t i = 0; i < scroll_offset; i++) {
        clear_start[i] = terminal_background_color;
    }
}