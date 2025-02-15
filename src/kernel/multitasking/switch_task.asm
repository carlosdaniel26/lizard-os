%define MAX_TASKS 8
%define STACK_END (DEFAULT_STACK_SIZE - 1)

; struct Task

%define KERNEL_STACK_TOP      0
%define VIRTUAL_ADDRESS_SPACE  4
%define NEXT_TASK             8
%define STATE                 12

%define EAX_OFFSET            32
%define EBX_OFFSET            36
%define ECX_OFFSET            40
%define EDX_OFFSET            44
%define ESI_OFFSET            48
%define EDI_OFFSET            52
%define EBP_OFFSET            56
%define ESP_OFFSET            60
%define EIP_OFFSET            64
%define EFLAGS_OFFSET         68

%define SCHEDULING_POLICY     16
%define SCHEDULING_PRIORITY   20
%define PID                   24
%define NAME                  28
%define CPU_TIME_CONSUMED     92


%define TASK_SIZE (CPU_TIME_CONSUMED + 4)