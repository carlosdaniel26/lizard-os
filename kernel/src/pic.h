#pragma once

#include <types.h>

void PIC_remap();
void PIC_unmaskIRQ(u8 irq);
void PIC_sendEOI(u8 irq);