#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../drivers/hd.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/kprint.h"
#include "../mm/alloc.h"
#include "../cpu/x86.h"

#include "mp.h"
#include "kernel.h"
#include "page.h"
#include "proc.h"
#include "lapic.h"

extern uint32_t tick;

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000

char hd_num = 0;
struct HD hd[HD_NUM];

void set_stack(uint32_t);

inline set_stack(uint32_t stack_addr)
{
  __asm__ volatile("movl %%eax, %%ebp" :: "a"(stack_addr));
  __asm__ volatile("movl %ebp, %esp");
  kprintf("debug ebp=%x esp=%x\n", ebp(), esp());
  hang();
}

void kernel_main() {
  kprintf("I am in kernel! ebp=%x esp=%x\n", ebp(), esp());
  isr_install();
  irq_install();

  //asm("int $2");
  //asm("int $3");

	hd_setup((void*)(SYSTEM_PARAM_ADDR));

  mpinit();

  //lapicinit();

  // phy memory mgr
  install_alloc();

  // start other processor
  startothers();

  // install virtual memory mangement
  install_page();
  kprintf("debug ebp=%x esp=%x\n", ebp(), esp());

  // 栈地址
  asm volatile("cli");
  asm volatile("movl %%eax, %%ebp" :: "a"(0x40000000));
  asm volatile("movl %ebp, %esp");
  asm volatile("sti");
  
  kprintf("debug ebp=%x esp=%x\n", ebp(), esp());

  //char *p = (char*)0x6000;
 // kprintf("ddd>%x ", p);
 // kprint_hex_n(p, 10);
 // kprintf("\n");

  //asm volatile("cli");
  //newb();

  kprint("los> ");
  // never return because we have change stack reg
  hang();
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
  //kprintf("mem:%x\n", *((uint32_t*)mem));
  kprintf("mem:");
  kprint_hex_n((char*)mem, 10);
  kprintf("\n");
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

  code = (unsigned char*)0x6000;

  for(c = cpus; c < cpus+ncpu; c++){
    kprintf("loop cpu: %x %x\n", c, mycpu());
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    // 第一个核使用0x90000，给它让出4K
    // 后面启动的核都使用0x90000 - 4K
    stack = (char*)0x8F000;
    *(void**)(code-4) = stack;
    *(void(**)(void))(code-8) = mpenter;

    lapicstartap(c->apicid, code);

    kprintf("wait for start cpu %d %x %x\n", c->apicid, stack, code);
    // wait for cpu to finish start
    while(c->started == 0) {};
    kprintf("finsih start cpu %d\n", c->apicid);
  }
}

void mpenter()
{
  struct cpu* cc = mycpu();
  kprintf("mpenter cpu:%d esp:%x cc:%x lapic:%x\n", cc->apicid, esp(), cc, lapic);
  install_page();
  // 栈地址
  asm volatile("cli");
  asm volatile("movl %%eax, %%edx" :: "a"(cc));
  asm volatile("movl %%eax, %%ebp" :: "a"(0x40000000));
  asm volatile("movl %ebp, %esp");
  asm volatile("pushl %edx");
  asm volatile("sti");
  kprintf("debug ebp=%x esp=%x\n", ebp(), esp());
  kprintf("debug >>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  // 这里不能调用mycpu() 因为其中的lapicid()会访问0xFEE00000, 对lapic不够熟悉
  //struct cpu* cc2 = mycpu();
  struct cpu* cc2 = (struct cpu*)(*(uint32_t*)(0x40000000 - 4));
  kprintf("debug>[%x] [%x] [%x] [%x]\n", lapic, cc, &cc, cc2);
  cc2->started = 1;
  hang();
}
