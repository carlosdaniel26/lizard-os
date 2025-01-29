section .bss
	align 16
stack_bottom:
	resb 16384 * 8  ; 128 KiB
stack_top: ; after the 128 KiB reserved for the stack

section .text
	global _start
_start:
	align 8

	; Multiboot2 header
multiboot2_header_start:
	dd 0xe85250d6         ; Magic number
	dd 0                  ; Architecture (0 for x86)
	dd multiboot2_header_end - multiboot2_header_start ; Header length
	dd -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header_start)) ; Checksum
multiboot2_header_end:

	; Tag: End
	align 8
	dw 0                  ; Type (END tag)
	dw 0                  ; Flags (no flags)
	dd 8                  ; Tag size (W + W + L = 8)

	; Configure stack
	mov esp, stack_top

	push ebx              ; multiboot_info
	push eax              ; magic_number

	; Call the kernel
	extern kernel_main
	call kernel_main

	; Clear interrupts and infinite loop to prevent the code from stopping
	cli
.hang:  
	hlt
	jmp .hang

times 510-($-$$) db 0  ; Fill up to 510 bytes
dw 0xAA55             ; Boot signature
