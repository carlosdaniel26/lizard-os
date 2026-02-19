#pragma once

#include <types.h>

void PIC_remap(void);
void PIC_unmaskIRQ(u8 irq);
void PIC_sendEOI(u8 irq);