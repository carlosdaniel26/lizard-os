#include <alias.h>
#include <ata.h>
#include <cpuid.h>
#include <fat16.h>
#include <framebuffer.h>
#include <gdt.h>
#include <helpers.h>
#include <idt.h>
#include <keyboard.h>
#include <limine.h>
#include <pic.h>
#include <pit.h>
#include <pmm.h>
#include <rtc.h>
#include <ss.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tty.h>
#include <vmm.h>

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request
    = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((used,
               section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

uint64_t stack_start;

void kmain()
{
    asm volatile("mov %%rsp, %0" : : "m"(stack_start) : "memory");

    stop_interrupts();
    save_boot_time();

    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        hlt();
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
    {
        hlt();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    setup_framebuffer(framebuffer->width, framebuffer->height, framebuffer->address,
                      framebuffer->pitch);
    init_cpuid();
    tty_initialize();
    pmm_init();
    init_gdt();
    init_idt();
    PIC_remap();
    init_keyboard();
    shit_shell_init();
    stop_interrupts();
    vmm_init();
    pit_init();
    task_init();
    ata_detect_devices();
    // start_interrupts();
    ATADevice *disk = ata_get(0);
    Fat16 fs;

    fat16_mount(&fs, disk);
    read_and_print_file(&fs, "carlos.txt");

    hlt();
}
