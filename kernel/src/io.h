#pragma once

#include <types.h>

u8 inb(u16 port);
u16 inw(u16 port);
u32 inl(u16 port);
void outb(u16 port, u8 value);
void outw(u16 port, u16 value);
void outl(u16 port, u32 value);

#define out_u8(port, value) outb((u16)(port), (u8)(value))
#define out_u16(port, value) outw((u16)(port), (u16)(value))
#define out_u32(port, value) outl((u16)(port), (u32)(value))

#define in_u8(port, value) inb((u16)(port))
#define in_u16(port, value) inw((u16)(port))
#define in_u32(port, value) inl((u16)(port))

void io_wait(void);
