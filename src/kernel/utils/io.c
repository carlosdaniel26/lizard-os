#include <stdint.h>

uint8_t inb(uint16_t port)
{
	uint8_t return_value;

	__asm__ volatile (
	"inb %w1, %b0"  /* Read a byte from the specified I/O port*/
				   : "=a"(return_value)	/* "=a" indicates that the returned value will be stored in the 'eax' register*/
				   : "Nd"(port)			/* "Nd" indicates that 'port' is an I/O port number that can be read as an integer*/
				   : "memory");			/* Informs the compiler that this instruction may access memory, preventing optimizations that could overlook this interaction*/

	return return_value;
}

uint16_t inw(uint16_t port)
{
	uint16_t return_value;

	__asm__ volatile (
	"inw %w1, %w0"  /* Read a byte from the specified I/O port*/
				   : "=a"(return_value)	/* "=a" indicates that the returned value will be stored in the 'eax' register*/
				   : "Nd"(port)			/* "Nd" indicates that 'port' is an I/O port number that can be read as an integer*/
				   : "memory");			/* Informs the compiler that this instruction may access memory, preventing optimizations that could overlook this interaction*/

	return return_value;
}

uint32_t inl(uint16_t port)
{
    uint32_t return_value;
    __asm__ volatile (
        "inl %1, %0"
        : "=a"(return_value)
        : "Nd"(port)
        : "memory"
    );
    return return_value;
}


void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile (
		"outb %b0, %w1"  /* Write a byte to the specified I/O port*/
						:				  /* No output operands*/
						: "a"(value),		/* Input operand: 'val' is loaded into the 'eax' register*/
						  "Nd"(port)	   /* Input operand: 'port' is an I/O port number that can be used as an integer*/
						: "memory"		 /* Informs the compiler that this instruction may modify memory, preventing certain optimizations*/
	);
}

void outw(uint16_t port, uint16_t value)
{
	__asm__ volatile (
		"outw %w0, %w1"  /* Write a byte to the specified I/O port*/
						:				  /* No output operands*/
						: "a"(value),		/* Input operand: 'val' is loaded into the 'eax' register*/
						  "Nd"(port)	   /* Input operand: 'port' is an I/O port number that can be used as an integer*/
						: "memory"		 /* Informs the compiler that this instruction may modify memory, preventing certain optimizations*/
	);
}

void outl(uint16_t port, uint32_t value)
{
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory"
    );
}


void io_wait(void)
{
	outb(0x80, 0);
}
