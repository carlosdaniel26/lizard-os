#include <alias.h>
#include <helpers.h>
#include <idt.h>
#include <init.h>
#include <io.h>
#include <ktime.h>
#include <pic.h>
#include <pit.h>
#include <sched.h>
#include <stdio.h>
#include <task.h>
#include <timer.h>

/* PIT operates in a 1.193.182 Hz frequency*/

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQUENCY_HZ 1193182

#define PIT_TARGET_HZ 1000 /* 1000 Hz = 1 ms tick */
#define PIT_DESIRED_FREQUENCY_HZ (PIT_FREQUENCY_HZ / PIT_TARGET_HZ)

#define PIT_VECTOR_INDEX 32

volatile u64 pit_ticks = 0;

void pit_stop(void)
{
#define PIC1_DATA 0x21
    u8 mask = inb(PIC1_DATA);
    mask |= 0x01;
    outb(PIC1_DATA, mask);
}

void pit_start(void)
{
    PIC_unmaskVector(PIT_VECTOR_INDEX);
}

int pit_init(void)
{
    outb(PIT_COMMAND, 0b00110110); /* Mode 3, Channel 0, low/high byte acess*/

    outb(PIT_CHANNEL0, PIT_DESIRED_FREQUENCY_HZ & 0xFF); /* Low Byte */
    outb(PIT_CHANNEL0, (PIT_DESIRED_FREQUENCY_HZ >> 8)); /* High Byte */

    isr_table[PIT_VECTOR_INDEX] = &isr_pit;

    return 0;
}

u64 pit_get_ticks(void)
{
    return pit_ticks;
}

static struct timer_driver pit_driver = {
    .name = "pit",
    .init = pit_init,
    .start = pit_start,
    .stop = pit_stop,
    .read = pit_get_ticks
};

static int __init pit_register()
{
    register_timer(&pit_driver);
    return 0;
}

core_initcall(pit_register);

void isr_pit(struct cpu_state *regs)
{
    pit_ticks++;

    time_tick_ns(1000000); /* 1 ms = 1,000,000 ns */
    task_tick();

    scheduler(regs);
    PIC_sendEOI(PIT_VECTOR_INDEX);
}