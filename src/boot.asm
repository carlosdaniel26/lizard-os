section .bss
align 16
stack_bottom:
	resb 16384 * 8  ; 128 KiB stack

section .multiboot
align 8

multiboot2_header_start:
	dd 0xe85250d6
	dd 0
	dd multiboot2_header_end - multiboot2_header_start
	dd -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start)) & 0xFFFFFFFF

multiboot2_header_end:
	
	; framebuffer_tag:
	dw 5                    ; Tag Type
	dw 0                    ; Flags
	dd 20                   ; Struct Size
	dd 1024                 ; Width of framebuffer (e.g., 1024)
	dd 768                  ; Height of framebuffer (e.g., 768)
	dd 32                   ; Depth of framebuffer (e.g., 32 bits per pixel)

	; end tag
	align 8
	dw 0  ; Type (END tag)
	dw 0  ; Flags
	dd 8  ; Size
	align 8

section .text
global _start
_start:
	mov esp, stack_bottom + (16384 * 8) - 16  ; Ensure proper stack alignment
	push ebx
	push eax
	extern kernel_main
	call kernel_main

	cli
	hlt
