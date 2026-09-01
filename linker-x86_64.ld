/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* Entry point: the hand-written stub in lizard/head.S */
ENTRY(_start)

/* Define the program headers we want so the loader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    head PT_LOAD FLAGS(5);           /* R E - lizard/head.S early stub */
    text PT_LOAD FLAGS(5);           /* R E */
    rodata PT_LOAD FLAGS(4);         /* R */
    data PT_LOAD FLAGS(6);           /* RW */
    init PT_LOAD FLAGS(7);           /* R W E - holds .init.text and writable .init.data */
}

SECTIONS
{
    /* Topmost 2 GiB of the address space, for -mcmodel=kernel. */
    . = 0xffffffff80000000;
    kernel_start = .;

    /* The early boot stub (head.S). e_entry points here; the loader jumps to
     * its physical copy, so it must be position-independent up to the CR3 load
     * and the absolute jump into the higher half. */
    .head : {
        KEEP(*(.head))
    } :head

    /* Initialization section */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __init_start = .;
    .init : {
        *(.init.text)
        *(.init.data)
        *(.init.const)

        . = ALIGN(8);
        __initcall_start = .;
        /* Level 0 - early */
        __initcall0_start = .;
        KEEP(*(SORT(.initcall0.*)))
        KEEP(*(.initcall0))
        __initcall0_end = .;
        /* Level 1 - core */
        __initcall1_start = .;
        KEEP(*(SORT(.initcall1.*)))
        KEEP(*(.initcall1))
        __initcall1_end = .;
        /* Level 2 - postcore */
        __initcall2_start = .;
        KEEP(*(SORT(.initcall2.*)))
        KEEP(*(.initcall2))
        __initcall2_end = .;
        /* Level 3 - arch */
        __initcall3_start = .;
        KEEP(*(SORT(.initcall3.*)))
        KEEP(*(.initcall3))
        __initcall3_end = .;
        /* Level 4 - subsys */
        __initcall4_start = .;
        KEEP(*(SORT(.initcall4.*)))
        KEEP(*(.initcall4))
        __initcall4_end = .;
        /* Level 5 - fs */
        __initcall5_start = .;
        KEEP(*(SORT(.initcall5.*)))
        KEEP(*(.initcall5))
        __initcall5_end = .;
        /* Level 6 - device */
        __initcall6_start = .;
        KEEP(*(SORT(.initcall6.*)))
        KEEP(*(.initcall6))
        __initcall6_end = .;
        /* Level 7 - late */
        __initcall7_start = .;
        KEEP(*(SORT(.initcall7.*)))
        KEEP(*(.initcall7))
        __initcall7_end = .;
        __initcall_end = .;
    } :init
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __init_end = .;

    .text : {
        *(.text .text.*)
    } :text

    /* Move to the next memory page for .rodata */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .rodata : {
        *(.rodata .rodata.*)

        . = ALIGN(16);
        /* Kernel parameters section for __setup macro */
        __setup_start = .;
        KEEP(*(.kernel_params))
        __setup_end = .;
    } :rodata

    /* Move to the next memory page for .data */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .data : {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } :data

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /* Discard .note.* and .eh_frame* since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }

    kernel_end = .;
}
