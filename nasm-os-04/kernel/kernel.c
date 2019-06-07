#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>
#include "../drivers/hd.h"


#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000

char hd_num = 0;
struct HD hd[HD_NUM];

void* hd_setup(void* addr) {
	hd_num = *((char*)addr);

	addr++;

	for (int i=0;i<hd_num;i++)
	{
		if (i<HD_NUM)
		{
			hd[i].cyl = *((unsigned int*)(addr+4));			
			hd[i].head = *((unsigned int*)(addr+8));			
			hd[i].sector = *((unsigned int*)(addr+12));			
			hd[i].nsectors = *((unsigned long*)(addr+16));			
			hd[i].sector_bytes = *((unsigned int*)(addr+24));			
		}
		addr += 30;
	}
	if (hd_num>HD_NUM)
		hd_num = HD_NUM;
	return addr;
}

void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

	hd_setup((void*)(SYSTEM_PARAM_ADDR));

}

void print_hd() {
	char hd_id_str[4];
	char cylinders_str[20];
	char heads_str[20];
	char sectors_str[20];
	char sector_bytes_str[10];
	char nsectors_str[20];
	kprint("hd cyl head sector sector_bytes nsectors\n");
	for (int i=0;i<hd_num;i++)
	{
		int_to_ascii(i,hd_id_str);
		kprint(hd_id_str);
		kprint(" ");
		int_to_ascii(hd[i].cyl,cylinders_str);
		kprint(cylinders_str);
		kprint(" ");
		int_to_ascii(hd[i].head,heads_str);
		kprint(heads_str);
		kprint(" ");
		int_to_ascii(hd[i].sector,sectors_str);
		kprint(sectors_str);
		kprint(" ");
		int_to_ascii(hd[i].sector_bytes,sector_bytes_str);
		kprint(sector_bytes_str);
		kprint(" ");
		int_to_ascii(hd[i].nsectors,nsectors_str);
		kprint(nsectors_str);
		kprint("\n");
	}

}

void user_input(char *input) {
    if (strcmp(input, "exit") == 0) {
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
    } else if (strcmp(input,"lshd")==0) {
		print_hd();
	} else if (strcmp(input,"readhd")==0) {
		hd_rw(37,HD_READ,1,(void*)(0x9200));
		kprint_k((char*)0x9200,10);
		
	}
    kprint("You said: ");
    kprint(input);
    kprint("\n> ");
}
