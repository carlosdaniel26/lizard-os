%define STUB_TABLE_ENTRIES 256

; Stubs 1 ... 256
; The pipeline will be:
; 1: idt_entry[x] -> interrupt_handler(x) (in C)
; 2: finish interrupt and IRET

%macro STUB_ENTRY 1
	global stub_%1
stub_%1:

	push %1						; push the interrupt id
	extern interrupt_handler
	call interrupt_handler		; call the real handler

	add esp, 4					; correct stack


	iret						; Retorna da interrupção
%endmacro

%macro GENERATE_STUBS 1
	%assign i 0
	%rep %1
		STUB_ENTRY i
	%assign i i+1
	%endrep
%endmacro

section .text
	GENERATE_STUBS STUB_TABLE_ENTRIES