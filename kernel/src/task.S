.section .note.GNU-stack,"",@progbits

.equ NAME, 0
.equ STATE, 32
.equ PRIORITY, 36
.equ TICKS_REMAINING, 40
.equ NEXT_TASK, 44
.equ CR3, 52
.equ STACK, 60
.equ RAX, 68
.equ RBX, 76
.equ RCX, 84
.equ RDX, 92
.equ RSI, 100
.equ RDI, 108
.equ RBP, 116
.equ RSP, 124
.equ R8,  132
.equ R9,  140
.equ R10, 148
.equ R11, 156
.equ R12, 164
.equ R13, 172
.equ R14, 180
.equ R15, 188
.equ RIP, 196
.equ REFLAGS, 204
.equ CS, 212
.equ DS, 220
.equ ES, 228
.equ FS, 236
.equ SS, 244


.data
	jmp_addr: .zero 8

.text

# void task_load_context(struct Task *task);
.global task_load_context
task_load_context:
    mov %rdi, %rax
    mov CR3(%rax), %rbx
    mov %rbx, %cr3
    mov RBX(%rax), %rbx
    mov RCX(%rax), %rcx
    mov RDX(%rax), %rdx
    mov RSI(%rax), %rsi
    mov RDI(%rax), %rdi
    mov RBP(%rax), %rbp
    mov R8(%rax),  %r8
    mov R9(%rax),  %r9
    mov R10(%rax), %r10
    mov R11(%rax), %r11
    mov R12(%rax), %r12
    mov R13(%rax), %r13
    mov R14(%rax), %r14
    mov R15(%rax), %r15

    mov RSP(%rax), %rsp
    push SS(%rax)
    push RSP(%rax)
    push REFLAGS(%rax)
    push CS(%rax)
    push RIP(%rax)
    iretq
