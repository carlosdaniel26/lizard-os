#pragma once

#include <types.h>

void PIC_remap();
void PIC_unmaskIRQ(u8 irq);
void PIC_unmaskVector(u8 vector);
void PIC_sendEOI(u8 irq);