global isr_pit
isr_pit:
    ; EOI
    mov al, 0x20
    out 0x20, al
    iret