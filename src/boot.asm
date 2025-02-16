section .bss
align 16
stack_bottom:
    resb 16384 * 8  ; 128 KiB

section .multiboot
align 8
multiboot2_header_start:
    dd 0xe85250d6
    dd 0
    dd multiboot2_header_end - multiboot2_header_start
    dd -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header_start))
multiboot2_header_end:

    dw 0
    dw 0
    dd 8

section .text
global _start
_start:
    mov esp, stack_bottom + (16384 * 8)
    push ebx
    push eax
    extern kernel_main
    call kernel_main

    cli
    hlt