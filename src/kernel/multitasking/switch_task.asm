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

section .data
    jmp_address: dd 0

section .text

global jump_to_task
; void jump_to_task(task* task)
jump_to_task:
    cli
    mov esi, [esp + 4]             ; task pointer
    mov eax, [esi + r_EIP]         ; EIP
    mov [jmp_address], eax         ; save EIP in jmp_address

    ; Load register values from task struct
    mov eax, [esi + r_EAX]
    mov ebx, [esi + r_EBX]
    mov ecx, [esi + r_ECX]
    mov edx, [esi + r_EDX]
    mov edi, [esi + r_EDI]
    mov ebp, [esi + r_EBP]
    mov esp, [esi + r_ESP]
    mov esi, [esi + r_ESI]

    ; Stack the task_exit function
    extern task_exit
    push task_exit 

    mov al, 0x20                    ; End of interrupt signal
    out 0x20, al                    ; Send the EOI to PIC

    sti                             ; Enable interrupts
    jmp [jmp_address]               ; Jump to saved EIP (task's entry point)
