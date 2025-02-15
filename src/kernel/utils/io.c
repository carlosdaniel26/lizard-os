#include <stdint.h>

uint8_t inb(uint16_t port) 
{
	uint8_t return_value;

	__asm__ volatile (
	"inb %w1, %b0"  // Read a byte from the specified I/O port
				   : "=a"(return_value)	// "=a" indicates that the returned value will be stored in the 'eax' register
				   : "Nd"(port)			// "Nd" indicates that 'port' is an I/O port number that can be read as an integer
				   : "memory");			// Informs the compiler that this instruction may access memory, preventing optimizations that could overlook this interaction

	return return_value;
}

void outb(uint16_t port, uint8_t value) 
{
	__asm__ volatile (
		"outb %b0, %w1"  // Write a byte to the specified I/O port
						:				  // No output operands
						: "a"(value),		// Input operand: 'val' is loaded into the 'eax' register
						  "Nd"(port)	   // Input operand: 'port' is an I/O port number that can be used as an integer
						: "memory"		 // Informs the compiler that this instruction may modify memory, preventing certain optimizations
	);
}

void io_wait(void)
{
	outb(0x80, 0);
}