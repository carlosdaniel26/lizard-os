#ifndef IO_H
#define IO_H

#include <stdint.h>

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);

#define out_u8(port, value)  outb((uint16_t)(port), (uint8_t)(value))
#define out_u16(port, value) outw((uint16_t)(port), (uint16_t)(value))
#define out_u32(port, value) outl((uint16_t)(port), (uint32_t)(value))

#define in_u8(port, value)  inb((uint16_t)(port))
#define in_u16(port, value) inw((uint16_t)(port))
#define in_u32(port, value) inl((uint16_t)(port))


void io_wait(void);

#endif
