#pragma once

int init_keyboard();
void isr_keyboard();

/* Raw scancode queue, drained by SYS_key_get. While raw mode is on the ISR
 * stops feeding the line-editor in tty.c, so a fullscreen program (doom) owns
 * the keyboard; SYS_key_get turns it on, task exit turns it back off. */
void keyboard_set_raw(int on);
int  keyboard_pop_scancode(void); /* set-1 scancode (bit7 = release), or -1 */
