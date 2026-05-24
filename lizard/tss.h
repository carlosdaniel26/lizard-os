#pragma once

#include <nolibc/types.h>

struct tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} __attribute__((packed));

void tss_init();
void tss_set_stack(u64 stack);
