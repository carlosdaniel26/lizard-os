/* userspace/lib/crt0.S - process entry. The kernel enters at _start with a
 * fresh stack and no arguments; we synthesise argc=0/argv=NULL, run main, and
 * turn its return value into _exit(). */
    .text
    .globl _start
    .type _start, @function
_start:
    xorl  %ebp, %ebp            /* terminate stack-unwind chains          */
    andq  $-16, %rsp            /* 16-byte align before the call          */

    xorl  %edi, %edi            /* argc = 0                               */
    xorl  %esi, %esi            /* argv = NULL                            */
    call  main

    movl  %eax, %edi            /* _exit(main())                          */
    call  _exit

.Lhang:                          /* _exit never returns; belt and braces  */
    hlt
    jmp   .Lhang
    .size _start, . - _start

    .section .note.GNU-stack, "", @progbits
