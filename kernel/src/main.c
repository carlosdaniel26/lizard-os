#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <framebuffer.h>
#include <tty.h>
#include <ss.h>
#include <rtc.h>
#include <pmm.h>
#include <gdt.h>
#include <idt.h>
#include <pic.h>
#include <cpuid.h>
#include <helpers.h>
#include <keyboard.h>

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

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kmain() 
{
    save_boot_time();

    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hlt();
    }

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hlt();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    setup_framebuffer(framebuffer->width, framebuffer->height, framebuffer->address, framebuffer->pitch);
    init_cpuid();
    tty_initialize();
    pmm_init();
    init_gdt();
    init_idt();
    PIC_remap();
    init_keyboard();
    shit_shell_init();
    //setup_paging();
    hlt();
}
