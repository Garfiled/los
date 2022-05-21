#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../drivers/hd.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/kprint.h"
#include "../mm/alloc.h"

#include "mp.h"
#include "kernel.h"
#include "page.h"
#include "proc.h"

#include <stdint.h>

extern uint32_t tick;

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000

char hd_num = 0;
struct HD hd[HD_NUM];

void kernel_main() {
  kprint("I am in kernel!\n");
  isr_install();
  irq_install();

  asm("int $2");
  asm("int $3");

	hd_setup((void*)(SYSTEM_PARAM_ADDR));

  // install virtual memory mangement
  install_alloc();
  install_page();

  mpinit();
  //lapicinit();

  char *p = (char*)0x8400;
  kprintf("ddd>%x ", p);
  kprint_hex_n(p, 10);
  kprintf("\n");
  startothers();

  kprint("los> ");
}

void* hd_setup(void* addr) {
  hd_num = *((char*)addr);
  kprintf("hd_setup hd_num:%d\n", hd_num);
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

void print_hd() {
	kprint("hd cyl head sector sector_bytes nsectors\n");
	for (int i = 0; i < hd_num; i++) {
    kprintf("%d %d %d %d %d %d\n", i, hd[i].cyl, hd[i].head, hd[i].sector, hd[i].sector_bytes, hd[i].nsectors);
	}
}
void print_mem(char s[])
{
	uint32_t mem = atoi(s);
  kprintf("mem:%x\n", *((uint32_t*)mem));
}

void read_hd(char s[])
{	
	int start_sector = atoi(s);
	//kprint_int(start_sector);
	//kprint("\n");
  void* buf = alloc_mm(512);
  if (buf != NULL) {
	  hd_rw(start_sector, HD_READ, 1, buf);
  } else {
    kprint("alloc mm fail");
  }
}

void write_hd(char s[])
{	
	int start_sector = atoi(s);
  char* buf = (char*)alloc_mm(512);
  if (buf == NULL) {
    kprint("alloc mm fail");
    return;
  }
  for (int i=0; i < 512; i++) {
    buf[i] = 'a';
  }
	//kprint(start_sector);
  //kprint("write_hd\n");
	hd_rw(start_sector, HD_WRITE, 1, buf);
}

void user_input(char *input) {
  if (strcmp(input, "exit") == 0) {
    kprint("Stopping the CPU. Bye!\n");
    asm volatile("hlt");
  } else if (strcmp(input, "lshd")==0) {
    print_hd();
	} else if (strcmpN(input, "readhd",6)==0) {
		read_hd(input+7);
	} else if (strcmpN(input, "checkhd",7)==0) {
		check_hd_status();
	} else if (strcmpN(input, "resethd",7)==0) {
		reset_hd_controller();
	} else if (strcmpN(input, "lsmem",5)==0) {
		print_mem(input+6);
	} else if (strcmpN(input, "writehd", 7) == 0) {
    write_hd(input + 8);
  } else if (strcmpN(input, "cls", 3) == 0) {
    clear_screen();
  } else if (strcmpN(input, "malloc", 6) == 0) {
    void *alloc_buf = alloc_mm(1);
    kprintf("-- alloc_buf=%x\n", alloc_buf);
  } else if (strcmpN(input, "write", 6) == 0) {
    char *buf = (char*)(16 * 1024 * 1024);
    buf[0] = 'a';
  }
  kprint("los> ");
}

// Start the non-boot (AP) processors.
void startothers(void)
{
  kprintf("startothers\n");
  unsigned char *code;
  struct cpu *c;
  char *stack;

  code = (unsigned char*)0x8400;

  for(c = cpus; c < cpus+ncpu; c++){
    kprintf("loop cpu: %x %x\n", c, mycpu());
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = alloc_mm(4096);
    *(void**)(code-4) = stack;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *)0x9000;

    lapicstartap(c->apicid, code);

    kprintf("wait for start cpu %d %x %x\n", c->apicid, stack, code);
    // wait for cpu to finish mpmain()
    while(c->started == 0)
      ;
    kprintf("finsih start cpu %d\n", c->apicid);
  }
}


void mpenter()
{
  kprintf("kernel cpu %d mpenter\n", mycpu()->apicid);
  mycpu()->started = 1;
  while (1) {
    asm volatile("hlt");
  }
}
