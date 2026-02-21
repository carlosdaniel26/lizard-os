/* TeleTYpewriter */

#pragma once

#include <stddef.h>
#include <types.h>
#include <vga.h>

#define TTY_DEFAULT_BG_COLOR VGA_COLOR_BLACK
#define TTY_DEFAULT_COLOR VGA_COLOR_WHITE

extern size_t terminal_width;
extern size_t terminal_height;

extern size_t terminal_text_width;
extern size_t terminal_text_height;

extern size_t terminal_row;
extern size_t terminal_column;
extern u32 tty_color;
extern u32 tty_bg_color;

extern size_t cmd_start_column;
extern size_t cmd_start_row;

void tty_initialize();
void tty_clean();
void tty_putentryat(char c, u32 color, size_t x, size_t y);
char tty_putchar(char c);
void tty_breakline();
void tty_write(const char *data, size_t size);
void tty_writestring(const char *data);
void tty_handler_input(char scancode);
void tty_backspace();
void tty_breakline();
void tty_tab();

/* KBDUS means US Keyboard Layout. This is a scancode table
 *	used to layout a standard US keyboard. I have left some
 *	comments in to give you an idea of what key is what, even
 *	though I set it's array index to 0. You can change that to
 *	whatever you want using a macro, if you wish! */

static const char convertScancode[128] = {
	0,	  27,  '1', '2', '3',  '4', '5', '6', '7',	'8', /* 9 */
	'9',  '0', '-', '=', '\b',							 /* Backspace */
	'\t',												 /* Tab */
	'q',  'w', 'e', 'r',								 /* 19 */
	't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',		 /* Enter key */
	0,													 /* 29	 - Control */
	'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',	';', /* 39 */
	'\'', '`', 0,										 /* Left shift */
	'\\', 'z', 'x', 'c', 'v',  'b', 'n',				 /* 49 */
	'm',  ',', '.', '/', 0,								 /* Right shift */
	'*',  0,											 /* Alt */
	' ',												 /* Space bar */
	0,													 /* Caps lock */
	0,													 /* 59 - F1 key ... > */
	0,	  0,   0,	0,	 0,	   0,	0,	 0,	  0,		 /* < ... F10 */
	0,													 /* 69 - Num lock*/
	0,													 /* Scroll Lock */
	0,													 /* Home key */
	0,													 /* Up Arrow */
	0,													 /* Page Up */
	'-',  0,											 /* Left Arrow */
	0,	  0,											 /* Right Arrow */
	'+',  0,											 /* 79 - End key*/
	0,													 /* Down Arrow */
	0,													 /* Page Down */
	0,													 /* Insert Key */
	0,													 /* Delete Key */
	0,	  0,   0,	0,									 /* F11 Key */
	0,													 /* F12 Key */
	0,													 /* All other keys are undefined */
};

