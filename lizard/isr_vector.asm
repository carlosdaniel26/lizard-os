section .note.GNU-stack noalloc noexec nowrite

default rel
extern isr_common_entry
extern syscall_handler_c

section .text

; Dedicated syscall entry (int 0x80). Unlike the generic per-vector stubs it
; must NOT clobber RDI before saving it - the syscall ABI passes arg1 in RDI.
global isr_syscall_stub
isr_syscall_stub:
    push qword 0x80         ; cpu_state.vec (keeps the frame layout uniform)
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

    mov rdi, rsp            ; struct cpu_state * -> arg0
    call syscall_handler_c

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

    add rsp, 8             ; discard the pushed vec
    iretq

; Common handler to save state and call C. The per-vector stub pushed the
; vector number just below the CPU's iret frame, so it lands in cpu_state.vec
; and no GPR (in particular RDI) is disturbed before it is saved.
isr_common_stub:
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

    mov rdi, [rsp + 15 * 8]   ; cpu_state.vec -> int_id (arg0)
    mov rsi, rsp              ; &cpu_state          -> arg1
    call isr_common_entry

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

    add rsp, 8               ; discard the pushed vector
    iretq

%macro STUB_ENTRY 1
global isr_vector_%1
isr_vector_%1:
    push qword %1
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