#include <types.h>

u8 inb(u16 port)
{
	u8 return_value;

	__asm__ volatile("inb %w1, %b0"		  /* Read a byte from the specified I/O port*/
					 : "=a"(return_value) /* "=a" indicates that the returned value will be stored
											 in the 'eax' register*/
					 : "Nd"(port) /* "Nd" indicates that 'port' is an I/O port number that can be
									 read as an integer*/
					 : "memory"); /* Informs the compiler that this instruction may access memory,
									 preventing optimizations that could overlook this interaction*/

	return return_value;
}

u16 inw(u16 port)
{
	u16 return_value;

	__asm__ volatile("inw %w1, %w0"		  /* Read a byte from the specified I/O port*/
					 : "=a"(return_value) /* "=a" indicates that the returned value will be stored
											 in the 'eax' register*/
					 : "Nd"(port) /* "Nd" indicates that 'port' is an I/O port number that can be
									 read as an integer*/
					 : "memory"); /* Informs the compiler that this instruction may access memory,
									 preventing optimizations that could overlook this interaction*/

	return return_value;
}

u32 inl(u16 port)
{
	u32 return_value;
	__asm__ volatile("inl %1, %0" : "=a"(return_value) : "Nd"(port) : "memory");
	return return_value;
}

void outb(u16 port, u8 value)
{
	__asm__ volatile(
		"outb %b0, %w1" /* Write a byte to the specified I/O port*/
		:				/* No output operands*/
		: "a"(value),	/* Input operand: 'val' is loaded into the 'eax' register*/
		  "Nd"(port) /* Input operand: 'port' is an I/O port number that can be used as an integer*/
		: "memory"	 /* Informs the compiler that this instruction may modify memory, preventing
						certain optimizations*/
	);
}

void outw(u16 port, u16 value)
{
	__asm__ volatile(
		"outw %w0, %w1" /* Write a byte to the specified I/O port*/
		:				/* No output operands*/
		: "a"(value),	/* Input operand: 'val' is loaded into the 'eax' register*/
		  "Nd"(port) /* Input operand: 'port' is an I/O port number that can be used as an integer*/
		: "memory"	 /* Informs the compiler that this instruction may modify memory, preventing
						certain optimizations*/
	);
}

void outl(u16 port, u32 value)
{
	__asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

void io_wait(void)
{
	outb(0x80, 0);
}