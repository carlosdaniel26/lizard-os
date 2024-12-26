ENTRY(_start)

SECTIONS
{
	. = 0x100000;

	kernel_start = .;

	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot2_header)
		*(.text)
	}

	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */

	kernel_end = .;

	kernel_size = kernel_start - kernel_end;
}
