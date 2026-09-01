/* userspace/lib/crt0.S - process entry. The kernel lays out a System V style
 * startup stack: [ argc | argv[0..] | NULL | envp NULL | auxv AT_NULL ] with
 * rsp pointing at argc and 16-byte aligned. We pass argc/argv to main and turn
 * its return value into _exit(). */
    .text
    .globl _start
    .type _start, @function
_start:
    xorl  %ebp, %ebp            /* terminate stack-unwind chains          */

    movq  (%rsp), %rdi          /* argc = [rsp]                           */
    leaq  8(%rsp), %rsi         /* argv = &[rsp+8]                        */

    andq  $-16, %rsp            /* keep 16-byte alignment before the call */
    call  main

    movl  %eax, %edi            /* _exit(main())                          */
    call  _exit

.Lhang:                          /* _exit never returns; belt and braces  */
    hlt
    jmp   .Lhang
    .size _start, . - _start

    .section .note.GNU-stack, "", @progbits
