#include <stdio.h>

#include <kernel/terminal/terminal.h>
#include <kernel/terminal/vga.h>
#include <kernel/cpu/pic.h>

#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/rtc.h>

// ************* ISR *******************
void isr_divide_by_zero() {
    terminal_setcolor(VGA_COLOR_RED);
    terminal_writestring("EXCEPTION: Divide by zero\n");
    asm volatile("cli; hlt");
}

void isr_invalid_opcode() {
    terminal_setcolor(VGA_COLOR_RED);
    terminal_writestring("EXCEPTION: Invalid opcode\n");
    asm volatile("cli; hlt");
}

void isr_page_fault() {
    terminal_setcolor(VGA_COLOR_RED);
    terminal_writestring("EXCEPTION: Page fault\n");
    asm volatile("cli; hlt");
}

void isr_stub_divide_by_zero() {
    // Push error code and call the actual ISR
    asm volatile("push $0");
    asm volatile("jmp isr_divide_by_zero");
}

void isr_stub_invalid_opcode() {
    // Push error code and call the actual ISR
    asm volatile("push $0");
    asm volatile("jmp isr_invalid_opcode");
}

void isr_stub_page_fault() {
    // Push error code and call the actual ISR
    asm volatile("push $0");
    asm volatile("jmp isr_page_fault");
}

void interrupt_handler(uint32_t interrupt_id)
{
    switch (interrupt_id)
    {
        case 0:
            isr_stub_divide_by_zero();
            break;
        
        case 6:
            isr_stub_invalid_opcode();
            break;
        
        case 14:
            isr_stub_page_fault();
            break;

        case 32:
            isr_timer();
            break;

        case 33:
            isr_keyboard();
            break;

        default:
            printf("unmapped interrupt: %u\n", interrupt_id);
            break;
    }

    PIC_sendEOI(1);
}