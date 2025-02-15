%define MAX_TASKS 8
%define STACK_END (DEFAULT_STACK_SIZE - 1)

; struct Task

%define KERNEL_STACK_TOP      0
%define VIRTUAL_ADDRESS_SPACE  4
%define NEXT_TASK             8
%define STATE                 12

%define EAX            32
%define EBX            36
%define ECX            40
%define EDX            44
%define ESI            48
%define EDI            52
%define EBP            56
%define ESP            60
%define EIP            64
%define EFLAGS         68

%define SCHEDULING_POLICY     16
%define SCHEDULING_PRIORITY   20
%define PID                   24
%define NAME                  28
%define CPU_TIME_CONSUMED     92


%define TASK_SIZE (CPU_TIME_CONSUMED + 4)

.bss:
	tasks:  resb (MAX_TASKS * TASK_SIZE) ; Alloc task[]

.text:

; int create_task(struct task *task, void (*entry_point)(void));
create_task:
	mov eax, [ebp + 8]	; task
	mov ebx, [ebp + 12]	; entry_point

	mov [eax + EIP], ebx
	ret