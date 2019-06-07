#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../libc/string.h"
#include "../libc/function.h"
#include "../kernel/kernel.h"
#include <stdint.h>

#define BACKSPACE 0x0E
#define ENTER 0x1C

static char key_buffer[256];

#define SC_MAX 57

enum {
	KEY_RSHIFT	= 0x36,		/* Right Shift */
	KEY_LSHIFT	= 0x2a,		/* Left Shift */
	KEY_NONE	= 0xff,		/* No key was pressed - NULL mark */
};

#define RELEASE(code)	(code | 0x80)

static char scancodes[][2] = {
	[0x01] = {  0 ,  0 , },	/* escape (ESC) */
	[0x02] = { '1', '!', },
	[0x03] = { '2', '@', },
	[0x04] = { '3', '#', },
	[0x05] = { '4', '$', },
	[0x06] = { '5', '%', },
	[0x07] = { '6', '^', },
	[0x08] = { '7', '&', },
	[0x09] = { '8', '*', },
	[0x0a] = { '9', '(', },
	[0x0b] = { '0', ')', },
	[0x0c] = { '-', '_', },
	[0x0d] = { '=', '+', },
	[0x0e] = { '\b', 0 , },	/* FIXME: VGA backspace support */
	[0x0f] = { '\t', 0 , },	/* FIXME: VGA tab support */
	[0x10] = { 'q', 'Q', },
	[0x11] = { 'w', 'W', },
	[0x12] = { 'e', 'E', },
	[0x13] = { 'r', 'R', },
	[0x14] = { 't', 'T', },
	[0x15] = { 'y', 'Y', },
	[0x16] = { 'u', 'U', },
	[0x17] = { 'i', 'I', },
	[0x18] = { 'o', 'O', },
	[0x19] = { 'p', 'P', },
	[0x1a] = { '[', '{', },
	[0x1b] = { ']', '}', },
	[0x1c] = { '\n', 0 , },	/* Enter */
	[0x1d] = {  0 ,  0 , },	/* Ctrl; good old days position */
	[0x1e] = { 'a', 'A', },
	[0x1f] = { 's', 'S', },
	[0x20] = { 'd', 'D', },
	[0x21] = { 'f', 'F', },
	[0x22] = { 'g', 'G', },
	[0x23] = { 'h', 'H', },
	[0x24] = { 'j', 'J', },
	[0x25] = { 'k', 'K', },
	[0x26] = { 'l', 'L', },
	[0x27] = { ';', ':', },	/* Semicolon; colon */
	[0x28] = { '\'', '"' }, /* Quote; doubelquote */
	[0x29] = { '`', '~', }, /* Backquote; tilde */
	[0x2a] = {  0,   0,  },	/* Left shift */
	[0x2b] = { '\\', '|' }, /* Backslash; pipe */
	[0x2c] = { 'z', 'Z', },
	[0x2d] = { 'x', 'X', },
	[0x2e] = { 'c', 'C', },
	[0x2f] = { 'v', 'V', },
	[0x30] = { 'b', 'B', },
	[0x31] = { 'n', 'N', },
	[0x32] = { 'm', 'M', },
	[0x33] = { ',', '<', },
	[0x34] = { '.', '>', },
	[0x35] = { '/', '?', },
	[0x36] = {  0 ,  0 , },	/* Right shift */
	[0x39] = { ' ', ' ', },	/* Space */
};


static int shifted;

static void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t scancode = port_byte_in(0x60);
	
	switch(scancode) {
	case KEY_LSHIFT:
	case KEY_RSHIFT:
		shifted = 1;
		break;
	case RELEASE(KEY_LSHIFT):
	case RELEASE(KEY_RSHIFT):
		shifted = 0;
		break;
	};    
	if (scancode > SC_MAX)
	{
		return;
	}
    if (scancode == BACKSPACE) {
        backspace(key_buffer);
        kprint_backspace();
    } else if (scancode == ENTER) {
        kprint("\n");
        user_input(key_buffer); /* kernel-controlled function */
        key_buffer[0] = '\0';
    } else {
        char letter = scancodes[(int)scancode][shifted];
		char str[2] = {letter, '\0'};
        append(key_buffer, letter);
		kprint(str);
    }
    UNUSED(regs);
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}
