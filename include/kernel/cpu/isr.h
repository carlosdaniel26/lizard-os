#ifndef ISR_H
#define ISR_H

/* ISRs reais (handlers para exceções)*/
void isr_divide_by_zero();
void isr_invalid_opcode();
void isr_page_fault();

/* Stubs para as exceções (entradas para a IDT)*/
void isr_stub_divide_by_zero();
void isr_stub_invalid_opcode();
void isr_stub_page_fault();

#endif /* ISR_H*/