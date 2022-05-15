#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../libc/string.h"
#include "../libc/function.h"
#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>

#define BACKSPACE 0x0E
#define ENTER 0x1C

static char key_buffer[256];

#define SC_MAX 57
const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar"};
const char sc_ascii[] =  {
         0 ,  0 , '1', '2',
        '3', '4', '5', '6',
        '7', '8', '9', '0',
        '-', '=',  0 ,  0 ,
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i',
        'o', 'p', '[', ']',
         0 ,  0 , 'a', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', ';',
        '\'','`',  0 , '\\',
        'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',',
        '.', '/',  0 , '*',
         0 , ' '
    }; 

static void keyboard_callback(registers_t *regs) {
  UNUSED(regs);
    /* The PIC leaves us the scancode in port 0x60 */
  uint8_t scancode = port_byte_in(0x60);
  static bool isLeftShiftPressed = false;
  static bool isRightShiftPressed = false;  
  if (scancode > SC_MAX) return;
  switch (scancode){
    case LeftShift:
      isLeftShiftPressed = true;
      break;
    case LeftShift + 0x80:
      isLeftShiftPressed = false;
      break;
    case RightShift:
      isRightShiftPressed = true;
      break;
    case RightShift + 0x80:
      isRightShiftPressed = false;
      break;
    case Enter:
      kprint("\n");
      user_input(key_buffer); /* kernel-controlled function */
      key_buffer[0] = '\0';
      break;
    case Spacebar:
      char buf[2] = {' ', '\0'};
      append(key_buffer, ' ');
      kprint(buf);
      break;
    case BackSpace:
      backspace(key_buffer);
      kprint_backspace();
      break;
    default:
      char letter;
      if (isLeftShiftPressed | isRightShiftPressed) {
        letter = sc_ascii[(int)scancode] -32;
      } else {
        letter = sc_ascii[(int)scancode];
      }
      /* Remember that kprint only accepts char[] */
      char str[2] = {letter, '\0'};
      append(key_buffer, letter);
      kprint(str);
    }
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}
