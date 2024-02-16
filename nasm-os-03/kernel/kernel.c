#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>
#include "../drivers/hd.h"

void kernel_main() {
  isr_install();
  irq_install();

  asm("int $2");
  asm("int $3");

  kprint("Type something, it will go through the kernel\n"
      "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

	 int* cylinders = (int*)0x9004;
	 int* heads = (int*)0x9008;
	 int* sectors = (int*)0x900c;
	 long* sectors_num = (long*)0x9010;
	 short* sectors_bytes = (short*)0x9018;
	 
	 char cylinders_str[20];
	 int_to_ascii(*cylinders,cylinders_str);
	 kprint("cylinders: ");
	 kprint(cylinders_str);
	 kprint(" ");
	 char cylinders_str_hex[20];
	 hex_to_ascii(*cylinders,cylinders_str_hex);
	 kprint(cylinders_str_hex);
	 kprint("\n");

	 char heads_str[20];
	 int_to_ascii(*heads,heads_str);
	 kprint("heads: ");
	 kprint(heads_str);
	 kprint(" ");
	 char heads_str_hex[20];
	 hex_to_ascii(*heads,heads_str_hex);
	 kprint(heads_str_hex);
	 kprint("\n");

	 char sectors_str[20];
	 int_to_ascii(*sectors,sectors_str);
	 kprint("sectors: ");
	 kprint(sectors_str);
	 kprint(" ");
	 char sectors_str_hex[20];
	 hex_to_ascii(*sectors,sectors_str_hex);
	 kprint(sectors_str_hex);
	 kprint("\n");

	 char sectors_num_str[50];
	 int_to_ascii(*sectors_num,sectors_num_str);
	 kprint("sectors_num: ");
	 kprint(sectors_num_str);
	 kprint(" ");
	 char sectors_bytes_str[20];
	 int_to_ascii(*sectors_bytes,sectors_bytes_str);
	 kprint(sectors_bytes_str);
	 kprint("\n");

	 kprint("HD_READ\n");
	 hd_rw(37,HD_READ,1,(void*)(0x9200));
	 
	 kprint_k((char*)0x9200,10);
}

void user_input(char *input) {
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
