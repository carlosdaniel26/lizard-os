#include <stdint.h>

void PIC_remap();
void PIC_unmaskIRQ(uint8_t irq); 
void PIC_sendEOI(uint8_t irq);