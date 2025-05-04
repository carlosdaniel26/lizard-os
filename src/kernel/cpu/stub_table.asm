; Process Trace struct offset

%define r_EBX    0
%define r_ECX    4
%define r_EDX    8
%define r_ESI    12
%define r_EDI    16
%define r_EBP    20
%define r_ESP    24
%define r_EIP    28
%define r_EFLAGS 32
%define r_EAX    36
%define r_DS     40
%define r_ES     44
%define r_FS     48
%define r_GS     52



%define STUB_TABLE_ENTRIES 256

section .data

global ptrace
ptrace:
	dd 52 + 4


section .text

; Stubs 1 ... 256
; The pipeline will be:
; 1: idt_entry[x] -> interrupt_handler(x) (in C)
; 2: finish interrupt and IRET

%macro STUB_ENTRY 1
	global stub_%1
stub_%1:

	; Save registers on ptrace
	mov [ptrace + r_EBX], ebx
	mov [ptrace + r_ECX], ecx
	mov [ptrace + r_EDX], edx
	mov [ptrace + r_ESI], esi
	mov [ptrace + r_EDI], edi
	mov [ptrace + r_EBP], ebp
	mov [ptrace + r_ESP], esp
	mov [ptrace + r_EAX], eax
	mov [ptrace + r_DS], ds
	mov [ptrace + r_ES], es
	mov [ptrace + r_FS], fs
	mov [ptrace + r_GS], gs
	mov [ptrace + r_EFLAGS], eax

	; Save the EIP return addr using eax as temp value
	push eax
	mov eax, [esp + 4]
	mov [ptrace + r_EIP], eax
	pop eax

	; Go to interrupt handler
	push %1						; push the interrupt id
	extern interrupt_handler
	call interrupt_handler		; call the real handler

	add esp, 4					; correct stack

	; Restore registers from ptrace
	mov     ebx, [ptrace + r_EBX]
	mov     ecx, [ptrace + r_ECX]
	mov     edx, [ptrace + r_EDX]
	mov     esi, [ptrace + r_ESI]
	mov     edi, [ptrace + r_EDI]
	mov     ebp, [ptrace + r_EBP]
	mov     esp, [ptrace + r_ESP]
	mov     eax, [ptrace + r_EAX]
	mov     ds,  [ptrace + r_DS]
	mov     es,  [ptrace + r_ES]
	mov     fs,  [ptrace + r_FS]
	mov     gs,  [ptrace + r_GS]

	mov     eax, [ptrace + r_EFLAGS]

	iret						; Interrupt return
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