/*
 * lizard/head.S - kernel entry from the UEFI loader.
 *
 * The loader jumps here (physical address, firmware identity map still active,
 * long mode already on) with RDI = struct boot_info *.
 *
 * _start runs position-independently at its physical load address: it builds a
 * fresh PML4 in the loader-provided scratch region with three ranges, all
 * 2 MiB pages (no CPU-feature dependency):
 *
 *   - identity   [0, 4 GiB)                      -> PML4[0]
 *   - HHDM       [0, roundup(highest_addr,1GiB)) -> PML4[hhdm_base >> 39]
 *   - kernel     [kernel_virt_base, +size)       -> PML4[511]
 *
 * then loads CR3 and does the absolute jump into the higher half (_start_high),
 * which sets the stack, publishes boot_info_ptr and calls kmain().
 *
 * boot_info field offsets (keep in sync with the _Static_asserts in boot.c):
 *   0x08 hhdm_base   0x10 highest_addr   0x18 kernel_phys_base
 *   0x20 kernel_virt_base   0x28 kernel_size
 *   0x30 scratch_phys_base  0x38 scratch_size
 */

#define BI_HHDM        0x08
#define BI_HIGHEST     0x10
#define BI_KPHYS       0x18
#define BI_KVIRT       0x20
#define BI_KSIZE       0x28
#define BI_SCRATCH     0x30
#define BI_SCRATCH_SZ  0x38

#define PTE_P   0x001
#define PTE_RW  0x002
#define PTE_PS  0x080
#define PAGE_2M 0x200000
#define KERNEL_STACK_SIZE 0x4000

    .section .head, "ax", @progbits
    .code64
    .globl _start
    .type _start, @function
_start:
    cli
    cld
    movq    %rdi, %r15                 /* r15 = boot_info* (preserved to _start_high) */

    /* temp stack at the top of the scratch region (head makes no calls, but be safe) */
    movq    BI_SCRATCH(%r15), %rsp
    addq    BI_SCRATCH_SZ(%r15), %rsp

    /* rbp = running bump-allocator over the (zeroed) scratch region */
    movq    BI_SCRATCH(%r15), %rbp

    /* --- PML4 --- */
    movq    %rbp, %rbx                 /* rbx = pml4 */
    addq    $0x1000, %rbp

    /* ===================== identity map [0, 4 GiB) ===================== */
    movq    %rbp, %r12                 /* r12 = pdpt_id */
    addq    $0x1000, %rbp
    movq    %r12, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, 0(%rbx)             /* pml4[0] -> pdpt_id */

    xorq    %rcx, %rcx                 /* rcx = 1 GiB index 0..3 */
.Lid_pdpt:
    movq    %rbp, %r13                 /* r13 = pd */
    addq    $0x1000, %rbp
    leaq    (%r12,%rcx,8), %rdx
    movq    %r13, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, (%rdx)             /* pdpt_id[rcx] -> pd */

    movq    %rcx, %rax
    shlq    $30, %rax                 /* rax = rcx * 1 GiB : base phys for this pd */
    xorq    %rsi, %rsi                 /* rsi = pd entry 0..511 */
.Lid_pd:
    movq    %rax, %rdx
    orq     $(PTE_P|PTE_RW|PTE_PS), %rdx
    movq    %rdx, (%r13,%rsi,8)
    addq    $PAGE_2M, %rax
    incq    %rsi
    cmpq    $512, %rsi
    jb      .Lid_pd

    incq    %rcx
    cmpq    $4, %rcx
    jb      .Lid_pdpt

    /* ===================== HHDM map [0, highest] at hhdm_base ===================== */
    movq    %rbp, %r11                 /* r11 = pdpt_hh */
    addq    $0x1000, %rbp
    movq    BI_HHDM(%r15), %rax
    shrq    $39, %rax
    andq    $0x1FF, %rax
    leaq    (%rbx,%rax,8), %rdx
    movq    %r11, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, (%rdx)             /* pml4[hhdm idx] -> pdpt_hh */

    movq    BI_HIGHEST(%r15), %rax
    addq    $0x3FFFFFFF, %rax
    shrq    $30, %rax                 /* rax = ceil(highest / 1 GiB) */
    testq   %rax, %rax
    jnz     1f
    movq    $1, %rax                  /* always map at least 1 GiB */
1:
    cmpq    $512, %rax
    jbe     2f
    movq    $512, %rax
2:
    movq    %rax, %r10                 /* r10 = number of 1 GiB PDs */
    xorq    %rcx, %rcx
.Lhh_pdpt:
    cmpq    %r10, %rcx
    jae     .Lhh_done
    movq    %rbp, %r13
    addq    $0x1000, %rbp
    leaq    (%r11,%rcx,8), %rdx
    movq    %r13, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, (%rdx)

    movq    %rcx, %rax
    shlq    $30, %rax
    xorq    %rsi, %rsi
.Lhh_pd:
    movq    %rax, %rdx
    orq     $(PTE_P|PTE_RW|PTE_PS), %rdx
    movq    %rdx, (%r13,%rsi,8)
    addq    $PAGE_2M, %rax
    incq    %rsi
    cmpq    $512, %rsi
    jb      .Lhh_pd

    incq    %rcx
    jmp     .Lhh_pdpt
.Lhh_done:

    /* ===================== kernel map [kernel_virt_base, +size) ===================== */
    movq    %rbp, %r9                  /* r9 = pdpt_k */
    addq    $0x1000, %rbp
    movq    %rbp, %r8                  /* r8 = pd_k */
    addq    $0x1000, %rbp

    movq    BI_KVIRT(%r15), %r14       /* r14 = kernel_virt_base */

    movq    %r14, %rax
    shrq    $39, %rax
    andq    $0x1FF, %rax
    leaq    (%rbx,%rax,8), %rdx
    movq    %r9, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, (%rdx)             /* pml4[pml4 idx] -> pdpt_k */

    movq    %r14, %rax
    shrq    $30, %rax
    andq    $0x1FF, %rax
    leaq    (%r9,%rax,8), %rdx
    movq    %r8, %rax
    orq     $(PTE_P|PTE_RW), %rax
    movq    %rax, (%rdx)             /* pdpt_k[pdpt idx] -> pd_k */

    movq    %r14, %rsi
    shrq    $21, %rsi
    andq    $0x1FF, %rsi               /* rsi = starting pd_k entry index */

    movq    BI_KPHYS(%r15), %rax       /* rax = running phys (2 MiB aligned by loader) */
    movq    BI_KSIZE(%r15), %r10
    addq    $(PAGE_2M-1), %r10
    shrq    $21, %r10                  /* r10 = number of 2 MiB pages to map */
.Lk_pd:
    testq   %r10, %r10
    jz      .Lk_done
    movq    %rax, %rdx
    orq     $(PTE_P|PTE_RW|PTE_PS), %rdx
    movq    %rdx, (%r8,%rsi,8)
    addq    $PAGE_2M, %rax
    incq    %rsi
    decq    %r10
    jmp     .Lk_pd
.Lk_done:

    /* ===================== switch ===================== */
    movq    %rbx, %cr3

    movw    $0x3F8, %dx               /* COM1 breadcrumb: 'C' (cr3 loaded) */
    movb    $0x43, %al
    outb    %al, %dx

    movabsq $_start_high, %rax
    jmp     *%rax
    .size _start, . - _start

    .globl _start_high
    .type _start_high, @function
_start_high:
    movabsq $kernel_stack, %rsp
    addq    $KERNEL_STACK_SIZE, %rsp
    andq    $~0xF, %rsp

    movabsq $boot_info_ptr, %rax
    movq    %r15, (%rax)

    movw    $0x3F8, %dx               /* COM1 breadcrumb: 'H' (higher half) */
    movb    $0x48, %al
    outb    %al, %dx

    xorl    %ebp, %ebp
    call    kmain
.Lhang:
    cli
    hlt
    jmp     .Lhang
    .size _start_high, . - _start_high

    .section .note.GNU-stack, "", @progbits
