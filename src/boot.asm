section .multiboot2_header
align 8
multiboot2_header_start:
    dd 0xe85250d6  ; Magic number
    dd 0           ; Architecture (0 para x86)
    dd multiboot2_header_end - multiboot2_header_start ; Header length
    dd -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header_start)) ; Checksum
multiboot2_header_end:

; Stack
section .bss
	align 16
	stack_bottom:
		resb 16384 * 8; 128 KiB 
	stack_top: ; after the 128 kib reserved for the stack

section .boot
global _start
_start:
	; configure stack
	mov esp, stack_top

    push ebx	; multiboot_info
    push eax	; magic_number
    ; Call the kernel
    extern kernel_main
	call kernel_main

	
; Clear interrupts and infinite loop just to code dont stop

	cli
.hang:	
	hlt
	jmp .hang

times 510-($-$$) db 0 ; Preencher até 510 bytes
dw 0xAA55              ; Boot signature	
