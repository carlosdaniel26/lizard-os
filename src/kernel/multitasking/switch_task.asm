extern pmm_alloc_block

%define MAX_TASKS 8
%define STACK_END (DEFAULT_STACK_SIZE - 1)

; struct Task

%define KERNEL_STACK_TOP      0
%define VIRTUAL_ADDRESS_SPACE  4
%define NEXT_TASK             8
%define STATE                 12

%define r_EAX            32
%define r_EBX            36
%define r_ECX            40
%define r_EDX            44
%define r_ESI            48
%define r_EDI            52
%define r_EBP            56
%define r_ESP            60
%define r_EIP            64
%define r_EFLAGS         68

%define SCHEDULING_POLICY     16
%define SCHEDULING_PRIORITY   20
%define PID                   24
%define NAME                  28
%define CPU_TIME_CONSUMED     92


%define TASK_SIZE (CPU_TIME_CONSUMED + 4)

global tasks

section .text:

; int create_task(struct task *task, void (*entry_point)(void));
global create_task
create_task:
	; Clean task struct
	

	mov ecx, [ebp + 4]	; task
	mov ebx, [ebp + 8]	; entry_point

	; alloc stack
	call pmm_alloc_block ; eax = addr


	; Create State
	mov [ecx + r_EIP], ebX		; task->EIP = entry_point
	mov [ecx + r_EBP], eaX		; task->EBP = stack_bottom
	mov [ecx + r_ESP], eax		; task->ESP = stack_bottom

	; Initialize General Registers to 0
	xor eax, eax
	mov [ecx + r_EAX], eax
	mov [ecx + r_EBX], eax
	mov [ecx + r_ECX], eax
	mov [ecx + r_EDX], eax
	mov [ecx + r_ESI], eax
	mov [ecx + r_EDI], eax


	ret
