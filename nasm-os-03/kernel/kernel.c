#include "cpu/isr.h"
#include "kernel/kernel.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "drivers/hd.h"
#include "drivers/screen.h"
#include <stdint.h>

void print_int(int val)
{
  char str[20] = {0};
  int_to_ascii(val, str);
  kprint(str);
}

void print_hex(int val)
{
  char str[20] = {0};
  hex_to_ascii(val, str);
  kprint(str);
}

void kernel_main()
{
  isr_install();
  irq_install();

  int* cylinders = (int*)0x9004;
  int* heads = (int*)0x9008;
  int* sectors = (int*)0x900c;
  long* sectors_num = (long*)0x9010;
  short* sectors_bytes = (short*)0x9018;

  kprint("cylinders: ");
  print_int(*cylinders);
  kprint(" ");
  print_hex(*cylinders);
  kprint("\n");

  kprint("heads: ");
  print_int(*heads);
  kprint(" ");
  print_hex(*heads);
  kprint("\n");

  kprint("sectors: ");
  print_int(*sectors);
  kprint(" ");
  print_hex(*sectors);
  kprint("\n");

  kprint("sectors_num: ");
  print_int(*sectors_num);
  kprint(" ");
  print_hex(*sectors_num);
  kprint("\n");

  kprint("sectors_bytes: ");
  print_int(*sectors_bytes);
  kprint(" ");
  print_hex(*sectors_bytes);
  kprint("\n");

  kprint("HD_READ\n");
  hd_rw(0, HD_READ, 1, (void*)(0x9200));

  // find magic number
  // use physical address is freedom for myself
  unsigned char *magic = (unsigned char*)0x9200 + 510;

  //kprint_k(magic, 2);
  //kprint("\n");
  unsigned int val = *magic;
  print_int(val);
  kprint("\n");
  print_hex(val);
  kprint("\n");
  val = *(magic + 1);
  print_int(val);
  kprint("\n");
  print_hex(val);
  kprint("\n");
}

void user_input(char *input)
{
  if (strcmp(input, "END") == 0) {
    kprint("Stopping the CPU. Bye!\n");
    asm volatile("hlt");
  } else if (strcmp(input, "PAGE") == 0) {
    /* Lesson 22: Code to test kmalloc, the rest is unchanged */
    uint32_t phys_addr;
    uint32_t page = kmalloc(1000, 1, &phys_addr);
    char page_str[16] = "";
    hex_to_ascii(page, page_str);
    char phys_str[16] = "";
    hex_to_ascii(phys_addr, phys_str);
    kprint("Page: ");
    kprint(page_str);
    kprint(", physical address: ");
    kprint(phys_str);
    kprint("\n");
  }
  kprint("You said: ");
  kprint(input);
  kprint("\n> ");
}
