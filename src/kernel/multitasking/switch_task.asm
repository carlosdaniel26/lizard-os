extern pmm_alloc_block

%define MAX_TASKS 8
%define STACK_END (DEFAULT_STACK_SIZE - 1)

; struct Task

%define KERNEL_STACK_TOP      0
%define VIRTUAL_ADDRESS_SPACE  4

%define NEXT_TASK             8
%define STATE                 12

%define r_EAX            16
%define r_EBX            20
%define r_ECX            24
%define r_EDX            28
%define r_ESI            32
%define r_EDI            36
%define r_EBP            40
%define r_ESP            44
%define r_EIP            48
%define r_EFLAGS         52

%define SCHEDULING_POLICY     56
%define SCHEDULING_PRIORITY   60
%define PID                   64
%define NAME                  68
%define CPU_TIME_CONSUMED     132

%define TASK_SIZE (CPU_TIME_CONSUMED + 4)