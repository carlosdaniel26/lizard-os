; flat assembler (fasm) source.
;
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

format ELF64

extrn isr_common_entry
extrn syscall_handler_c

public isr_syscall_stub
public isr_common_stub
public isr_stub_table

section '.text' executable

macro SAVE_GPRS
{
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
}

macro RESTORE_GPRS
{
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
}

; Dedicated syscall entry (int 0x80). int 0x80 never pushes an error code.
isr_syscall_stub:
    push 0                 ; dummy errcode
    push 0x80              ; vec
    SAVE_GPRS

    mov rdi, rsp           ; struct cpu_state * -> arg0
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

; The generated stubs are only reached through isr_stub_table below, so their
; exact label names do not matter.
rept 256 vec:0
{
    isr_vector_#vec:
    if vec = 8 | vec = 10 | vec = 11 | vec = 12 | vec = 13 | vec = 14 | vec = 17 | vec = 21 | vec = 29 | vec = 30
        ; the CPU already pushed an error code for this vector
    else
        push 0            ; dummy error code
    end if
    push vec             ; vector number
    jmp isr_common_stub
}

section '.data' writeable

isr_stub_table:
rept 256 vec:0
{
    dq isr_vector_#vec
}

section '.note.GNU-stack'
