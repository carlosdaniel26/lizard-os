#include <lizard/debug.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/io.h>
#include <lizard/keyboard.h>
#include <lizard/pic.h>
#include <lizard/setup.h>
#include <nolibc/stdio.h>
#include <lizard/tty.h>
#include <nolibc/types.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_VECTOR 33

/* Raw scancode ring. Single producer (ISR), single consumer (SYS_key_get). */
#define KBD_RING_SIZE 256
static volatile u8 kbd_ring[KBD_RING_SIZE];
static volatile u32 kbd_head; /* write index */
static volatile u32 kbd_tail; /* read index  */
static volatile int kbd_raw;

void keyboard_set_raw(int on)
{
    kbd_raw = on ? 1 : 0;
    if (!on)
        kbd_tail = kbd_head; /* drop anything the fullscreen app never read */
}

int keyboard_pop_scancode(void)
{
    if (kbd_tail == kbd_head)
        return -1;
    u8 sc = kbd_ring[kbd_tail % KBD_RING_SIZE];
    kbd_tail++;
    return (int)sc;
}

static void kbd_ring_push(u8 sc)
{
    u32 next = kbd_head + 1;
    if (next - kbd_tail > KBD_RING_SIZE)
        kbd_tail++; /* overwrite the oldest */
    kbd_ring[kbd_head % KBD_RING_SIZE] = sc;
    kbd_head = next;
}

int init_keyboard()
{
    /* Drain whatever the firmware left in the i8042 output buffer. While OBF
     * (bit 0 of the status port) stays set the controller won't raise IRQ1, so
     * a single stale byte here means the first keystrokes after boot are lost. */
    while (inb(0x64) & 0x01)
        (void)inb(KEYBOARD_DATA_PORT);

    isr_table[KEYBOARD_VECTOR] = &isr_keyboard;
    PIC_unmaskVector(KEYBOARD_VECTOR);
    kprintf("keyboard initialized\n");
    return 0;
}

device_initcall(init_keyboard);

void isr_keyboard(struct cpu_state *regs)
{
    (regs);
    while (inb(0x64) & 0x01)
    {
        u8 scancode = inb(KEYBOARD_DATA_PORT);

        if (kbd_raw)
            kbd_ring_push(scancode);
        else
            tty_handler_input(scancode);
    }
    PIC_sendEOI(1);
}
