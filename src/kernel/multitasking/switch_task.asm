extern pmm_alloc_block

%define MAX_TASKS 8
%define STACK_END (DEFAULT_STACK_SIZE - 1)

; struct Task layout

%define STACK_TOP                0
%define VIRTUAL_ADDRESS_SPACE    4

%define PREV_TASK                8
%define NEXT_TASK                12
%define STATE                    16

%define r_EAX                   20
%define r_EBX                   24
%define r_ECX                   28
%define r_EDX                   32
%define r_ESI                   36
%define r_EDI                   40
%define r_EBP                   44
%define r_ESP                   48
%define r_EIP                   52
%define r_EFLAGS                56

%define SCHEDULING_POLICY       60
%define SCHEDULING_PRIORITY     64
%define PID                     68
%define NAME                    72
%define CPU_TIME_CONSUMED       136

%define TASK_SIZE (CPU_TIME_CONSUMED + 4)   ; = 140 bytes

.data
    jmp_address: dd 0



.text
global jump_to_task
; void jump_to_task(task* task)
jump_to_task:
    mov esi, [esp + 4] ; task pointer
    mov eax, [esi + r_EIP] ; EIP
    mov [jmp_address], eax ; save EIP on stack overwriting task pointer

    mov eax, [esi + r_EAX]
    mov ebx, [esi + r_EBX]
    mov ecx, [esi + r_ECX]
    mov edx, [esi + r_EDX]
    mov edi, [esi + r_EDI]
    mov ebp, [esi + r_EBP]
    mov esp, [esi + r_ESP]

    mov esi, [esi + r_ESI]
    jmp [jmp_address]