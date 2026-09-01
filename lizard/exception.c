#include <lizard/exception.h>
#include <lizard/gdt.h>
#include <lizard/init.h>
#include <lizard/panic.h>
#include <lizard/sched.h>
#include <lizard/task.h>
#include <nolibc/stdio.h>
#include <nolibc/types.h>

extern u32 tty_color;
extern u8 scheduler_enabled;

#define HIGH_HALF_MIN 0xFFFF800000000000ULL

static const char *exc_name(u64 v)
{
    switch (v)
    {
    case 0:  return "#DE Divide Error";
    case 1:  return "#DB Debug";
    case 2:  return "NMI";
    case 3:  return "#BP Breakpoint";
    case 4:  return "#OF Overflow";
    case 5:  return "#BR Bound Range";
    case 6:  return "#UD Invalid Opcode";
    case 7:  return "#NM Device Not Available";
    case 8:  return "#DF Double Fault";
    case 10: return "#TS Invalid TSS";
    case 11: return "#NP Segment Not Present";
    case 12: return "#SS Stack Fault";
    case 13: return "#GP General Protection";
    case 14: return "#PF Page Fault";
    case 16: return "#MF x87 FP";
    case 17: return "#AC Alignment Check";
    case 18: return "#MC Machine Check";
    case 19: return "#XM SIMD FP";
    default: return "Exception";
    }
}

static u64 read_cr2(void)
{
    u64 v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

static void dump_regs(struct cpu_state *r)
{
    kprintf("  RIP=%p  CS=%x  RFLAGS=%p\n", (void *)r->rip, (unsigned)r->cs, (void *)r->rflags);
    kprintf("  RSP=%p  SS=%x  errcode=%x\n", (void *)r->rsp, (unsigned)r->ss, (unsigned)r->errcode);
    kprintf("  RAX=%p RBX=%p RCX=%p RDX=%p\n", (void *)r->rax, (void *)r->rbx, (void *)r->rcx, (void *)r->rdx);
    kprintf("  RSI=%p RDI=%p RBP=%p\n", (void *)r->rsi, (void *)r->rdi, (void *)r->rbp);
    kprintf("  R8 =%p R9 =%p R10=%p R11=%p\n", (void *)r->r8, (void *)r->r9, (void *)r->r10, (void *)r->r11);
    kprintf("  R12=%p R13=%p R14=%p R15=%p\n", (void *)r->r12, (void *)r->r13, (void *)r->r14, (void *)r->r15);

    if (r->vec == 14)
    {
        u64 e = r->errcode;
        kprintf("  CR2=%p  [%s | %s | %s%s%s]\n", (void *)read_cr2(),
                (e & 1) ? "protection" : "not-present",
                (e & 2) ? "write" : "read",
                (e & 4) ? "user" : "kernel",
                (e & 8) ? " | reserved-bit" : "",
                (e & 16) ? " | instr-fetch" : "");
    }
}

/* Kernel is built -O0, so RBP is a real frame pointer: [rbp]=caller rbp,
 * [rbp+8]=return address. Only follow frames that stay in the higher half and
 * keep climbing. */
static void backtrace(u64 rbp)
{
    kprintf("  backtrace:\n");
    for (int i = 0; i < 20; i++)
    {
        if (rbp < HIGH_HALF_MIN || (rbp & 0x7))
            break;

        u64 *frame = (u64 *)rbp;
        u64 ret = frame[1];
        u64 next = frame[0];

        if (ret < HIGH_HALF_MIN)
            break;
        kprintf("    %p\n", (void *)ret);

        if (next <= rbp) /* stack grows down; a valid caller frame is above us */
            break;
        rbp = next;
    }
}

void exception_handle(struct cpu_state *regs)
{
    int from_user = (regs->cs & 3) == 3;
    u32 saved_color = tty_color;

    tty_color = VGA_COLOR_RED;
    kprintf("\n*** EXCEPTION %u  %s ***\n", (unsigned)regs->vec, exc_name(regs->vec));
    tty_color = saved_color;

    dump_regs(regs);
    backtrace(regs->rbp);

    if (from_user && scheduler_enabled && current_task && current_task != &idle)
    {
        kprintf("  killing task '%s' (pid %u)\n", current_task->name, (unsigned)current_task->pid);
        current_task->exit_code = -((int)regs->vec); /* negative = died on a signal-ish */

        /* Return into the kernel: rewrite the trap frame so the iretq performs
         * a clean CPL3 -> CPL0 switch onto this task's kernel stack, landing in
         * task_exit() (marks TERMINATED and reschedules; never returns). */
        regs->cs = KERNEL_CS;
        regs->ss = KERNEL_SS;
        regs->rsp = current_task->kernel_stack;
        regs->rflags = 0x2; /* IF cleared while we reap */
        regs->rip = (u64)task_exit;
        return;
    }

    kpanic("unrecoverable %s in the kernel", exc_name(regs->vec));
}
