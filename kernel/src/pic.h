#include <stdint.h>

void pic_remap(int offset1, int offset2); 
void pic_unmask_irq(uint8_t irq);
void pic_send_eoi(uint8_t irq);