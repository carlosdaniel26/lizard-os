#include <tss.h>
#include <gdt.h>
#include <string.h>

static struct tss main_tss;

void tss_init()
{
    memset(&main_tss, 0, sizeof(struct tss));
    main_tss.iopb_offset = sizeof(struct tss);

    int index = gdt_add_tss_gate((u64)&main_tss, sizeof(struct tss) - 1, 0x89, 0);

    asm volatile("ltr %%ax" : : "a"((u16)(index * 8)));
}

void tss_set_stack(u64 stack)
{
    main_tss.rsp0 = stack;
}
