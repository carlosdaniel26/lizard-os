/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* We want the symbol kmain to be our entry point */
ENTRY(kmain)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    /* We want to be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    . = 0xffffffff80000000;
    kernel_start = .;

    /* Define a section to contain the Limine requests and assign it to its own PHDR */
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    /* Move to the next memory page for .text */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
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

        . = ALIGN(8);
        __init.const_start = .;
        KEEP(*(.init.const))
        __init.const_end = .;
        
        /* Initcalls */
        . = ALIGN(8);
        __initcall_start = .;

        /* Level 0 - early */
        . = ALIGN(8);
        __initcall0_start = .;
        KEEP(*(SORT(.initcall0.*)))
        KEEP(*(.initcall0))
        __initcall0_end = .;

        /* Level 1 - core */
        . = ALIGN(8);
        __initcall1_start = .;
        KEEP(*(SORT(.initcall1.*)))
        KEEP(*(.initcall1))
        __initcall1_end = .;

        /* Level 2 - postcore */
        . = ALIGN(8);
        __initcall2_start = .;
        KEEP(*(SORT(.initcall2.*)))
        KEEP(*(.initcall2))
        __initcall2_end = .;

        /* Level 3 - arch */
        . = ALIGN(8);
        __initcall3_start = .;
        KEEP(*(SORT(.initcall3.*)))
        KEEP(*(.initcall3))
        __initcall3_end = .;

        /* Level 4 - subsys */
        . = ALIGN(8);
        __initcall4_start = .;
        KEEP(*(SORT(.initcall4.*)))
        KEEP(*(.initcall4))
        __initcall4_end = .;

        /* Level 5 - fs */
        . = ALIGN(8);
        __initcall5_start = .;
        KEEP(*(SORT(.initcall5.*)))
        KEEP(*(.initcall5))
        __initcall5_end = .;

        /* Level 6 - device */
        . = ALIGN(8);
        __initcall6_start = .;
        KEEP(*(SORT(.initcall6.*)))
        KEEP(*(.initcall6))
        __initcall6_end = .;

        /* Level 7 - late */
        . = ALIGN(8);
        __initcall7_start = .;
        KEEP(*(SORT(.initcall7.*)))
        KEEP(*(.initcall7))
        __initcall7_end = .;

        __initcall_end = .;
    } :rodata

    /* Move to the next memory page for .data */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .data : {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;

        . = ALIGN(8);
        __init.data_start = .;
        KEEP(*(.init.data))
        __init.data_end = .;
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
