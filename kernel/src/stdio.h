#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <vga.h>

extern uint32_t terminal_color;

#define EOF (-1)
typedef int (*pfnStreamWriteBuf)(char*);

bool kprint(const char* data, size_t length);
int kprintf(const char* __restrict, ...);
#define debug_printf(fmt, ...) \
	kprintf("[DEBUG] %s:%u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define kpanic(fmt, ...) \
	uint32_t temp = terminal_color; \
	terminal_color = VGA_COLOR_RED; \
	kprintf("[PANIC]"); \
	terminal_color = temp; \
	kprintf("%s:%u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

int putchar(char character);
void dd(const char * restrict format, ...);