section .note.GNU-stack noalloc noexec nowrite

default rel
extern isr_common_entry
extern syscall_handler_c

section .text

; Every entry path builds the same on-stack layout (matches struct cpu_state):
;
;   [ss][rsp][rflags][cs][rip]   <- pushed by the CPU
;   [errcode]                    <- CPU for #DF/#TS/#NP/#SS/#GP/#PF/#AC/#CP,
;                                   otherwise a dummy 0 pushed by the stub
;   [vec]                        <- pushed by the stub
;   [r15 ... rax]                <- pushed by SAVE_GPRS
;
; No GPR (RDI in particular, which carries syscall arg1) is disturbed before it
; is saved, and the C side always sees a valid vec + errcode.

%macro SAVE_GPRS 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

%macro RESTORE_GPRS 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

; Dedicated syscall entry (int 0x80). int 0x80 never pushes an error code.
global isr_syscall_stub
isr_syscall_stub:
    push qword 0            ; dummy errcode
    push qword 0x80         ; vec
    SAVE_GPRS

    mov rdi, rsp            ; struct cpu_state * -> arg0
    call syscall_handler_c

    RESTORE_GPRS
    add rsp, 16            ; discard vec + errcode
    iretq

isr_common_stub:
    SAVE_GPRS

    mov rdi, [rsp + 15 * 8] ; cpu_state.vec -> int_id (arg0)
    mov rsi, rsp            ; &cpu_state          -> arg1
    call isr_common_entry

    RESTORE_GPRS
    add rsp, 16            ; discard vec + errcode
    iretq

%macro STUB_ENTRY 1
global isr_vector_%1
isr_vector_%1:
%if (%1 == 8) || (%1 == 10) || (%1 == 11) || (%1 == 12) || (%1 == 13) || (%1 == 14) || (%1 == 17) || (%1 == 21) || (%1 == 29) || (%1 == 30)
    ; the CPU already pushed an error code for this vector
%else
    push qword 0           ; dummy error code
%endif
    push qword %1          ; vector number
    jmp isr_common_stub
%endmacro

%assign i 0
%rep 256
    STUB_ENTRY i
    %assign i i+1
%endrep

section .data
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr_vector_%[i]
    %assign i i+1
%endrep
