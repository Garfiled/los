#include "cpu/isr.h"
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "kernel/kernel.h"
#include "libc/string.h"
#include "libc/mem.h"

#include <stdint.h>

extern uint32_t tick;

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000

char hd_num = 0;
struct HD hd[HD_NUM];

void* hd_setup(void* addr)
{
  kprint("hd_setup\n");
  hd_num = *((char*)addr);
  kprint("hd_num:");
  kprint_int(hd_num);
  kprint("\n");
  addr++;

  for (int i=0;i<hd_num;i++) {
    if (i<HD_NUM) {
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

void kernel_main()
{
  kprint("I am in kernel!\n");
  isr_install();
  irq_install();

  hd_setup((void*)(SYSTEM_PARAM_ADDR));
}

void print_hd()
{
  char hd_id_str[4];
  char cylinders_str[20];
  char heads_str[20];
  char sectors_str[20];
  char sector_bytes_str[10];
  char nsectors_str[20];
  kprint("hd cyl head sector sector_bytes nsectors\n");
  for (int i = 0; i < hd_num; i++) {
  	int_to_ascii(i, hd_id_str);
  	kprint(hd_id_str);
  	kprint(" ");
  	int_to_ascii(hd[i].cyl, cylinders_str);
  	kprint(cylinders_str);
  	kprint(" ");
  	int_to_ascii(hd[i].head, heads_str);
  	kprint(heads_str);
  	kprint(" ");
  	int_to_ascii(hd[i].sector, sectors_str);
  	kprint(sectors_str);
  	kprint(" ");
  	int_to_ascii(hd[i].sector_bytes, sector_bytes_str);
  	kprint(sector_bytes_str);
  	kprint(" ");
  	int_to_ascii(hd[i].nsectors, nsectors_str);
  	kprint(nsectors_str);
  	kprint("\n");
  }
}

void print_mem(char s[])
{
  int mem_addr = 0;
  if (strlen(s) >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
    mem_addr = hex_to_int(&s[2]);
  } else {
    mem_addr = atoi(s);
  }
  char mem_str[20];
  hex_to_ascii(mem_addr, mem_str);
  kprint(mem_str);
  kprint(":");
  char tmp_str[20];
  for (int i = 0; i < 10; i++) {
  	tmp_str[0] = 0;
  	hex_to_ascii_inner(((unsigned char*)mem_addr)[i], tmp_str);
  	kprint(tmp_str);
  	//kprint(" ");
  }
  kprint("\n");
}

void read_hd(char s[])
{
  int start_sector = atoi(&s[2]);
  if (s[0] == '0') {
    hd_rw(true, start_sector, HD_READ, 1, (void*)(0x9200));
  } else if (s[1] == '1') {
    hd_rw(false, start_sector, HD_READ, 1, (void*)(0x9200));
  }
}

void write_hd(char s[])
{
  int start_sector = atoi(&s[2]);
  char *buf = (char*)0x9200;
  for (int i=0; i < 512; i++) {
    buf[i] = 'a';
  }
  if (s[0] == '0') {
    hd_rw(true, start_sector, HD_WRITE, 1, buf);
  } else if (s[1] == '1') {
    hd_rw(false, start_sector, HD_WRITE, 1, buf);
  }
}

void user_input(char *input) {
  if (strcmp(input, "exit") == 0) {
    kprint("Stopping the CPU. Bye!\n");
    asm volatile("hlt");
  } else if (strcmp(input, "page") == 0) {
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
  } else if (strcmpN(input,"readhd",6)==0) {
    read_hd(input + 7);
  } else if (strcmpN(input, "checkhd",7)==0) {
    check_hd_status();
  } else if (strcmpN(input,"resethd",7)==0) {
    reset_hd_controller();
  } else if (strcmpN(input,"lsmem",5)==0) {
    print_mem(input+6);
  } else if (strcmpN(input, "writehd", 7) == 0) {
    write_hd(input + 8);
  } else if (strcmpN(input, "cls", 3) == 0) {
    clear_screen();
  }
  kprint("You said: ");
  kprint(input);
  kprint("\n> ");
}
