#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <framebuffer.h>

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static void hlt() 
{
    for (;;) {
        asm ("hlt");
    }
}

void kmain() 
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hlt();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hlt();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    setup_framebuffer(framebuffer->width, framebuffer->height, framebuffer->address);

    clear_framebuffer();

    const char str[] = "Welcome to lizard-OS";

    for (size_t i = 0; i < sizeof(str)-1; i++)
    {
        draw_char(FONT_WIDTH * i, 0, 0, str[i]);
    }

    hlt();
}
