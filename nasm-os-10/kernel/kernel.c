#include "cpu/isr.h"
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "libc/kprint.h"
#include "mm/alloc.h"
#include "cpu/x86.h"
#include "fs/vfs.h"

#include "kernel/mp.h"
#include "kernel/kernel.h"
#include "kernel/syscall.h"
#include "kernel/page.h"
#include "kernel/proc.h"
#include "kernel/lapic.h"

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000
#define START_CORE_SIMPLE_PG 0x400000

char hd_num = 0;
struct HD hd[HD_NUM];

void kernel_main() {
  kprintf("I am in kernel! ebp=%x esp=%x\n", ebp(), esp());
  isr_install();
  irq_install();

  init_syscall();

	hd_setup((void*)(SYSTEM_PARAM_ADDR));

  mpinit();

  //lapicinit();

  // phy memory mgr
  install_alloc();

  // 初始化processor启动过渡页表
  init_entry_page();

  // start other processor
  startothers();

  set_cr3((uint32_t)entry_pg_dir);
  kprintf("cr3:%x esp:%x\n",cr3(), esp());

  open_mm_page();
  register_interrupt_handler(14, page_fault_handler);

  // 初始化文件系统
  init_file_system();

  process_exec("hell", 0, NULL);
  scheduler();
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

void read_hd_cmd(char s[])
{
  int start_sector = atoi(&s[2]);
  void* buf = alloc_mm(512);
  bool is_master_device = false;
  if (s[0] == '0') {
    is_master_device = true;
  } else if (s[1] == '1') {
    is_master_device = false;
  }
  hd_rw(is_master_device, start_sector, HD_READ, 1, buf);
}

void write_hd(char s[])
{
  int start_sector = atoi(&s[2]);
  char *buf = (char*)alloc_mm(512);
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
  } else if (strcmp(input, "lshd")==0) {
    print_hd();
	} else if (strcmpN(input, "readhd", 6)==0) {
		read_hd_cmd(input+7);
	} else if (strcmpN(input, "checkhd", 7)==0) {
		check_hd_status();
	} else if (strcmpN(input, "resethd", 7)==0) {
		reset_hd_controller();
	} else if (strcmpN(input, "lsmem", 5)==0) {
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
    static int offset = 0;
    offset++;
    stack = (char*)(START_CORE_SIMPLE_PG - offset * 4096);
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
  set_cr3((uint32_t)entry_pg_dir);
  open_mm_page();

  cc->started = 1;
  scheduler();
}

// boot page table
__attribute__((__aligned__(4096)))
uint32_t entry_pg_dir[1024];

__attribute__((__aligned__(4096)))
uint32_t entry_pg_table[1024];

__attribute__((__aligned__(4096)))
uint32_t reserve_for_map[1024];

void init_entry_page()
{
  MEMSET(entry_pg_dir, 0 , 4096);
  MEMSET(entry_pg_table, 0 , 4096);
  MEMSET(reserve_for_map, 0 , 4096);
  entry_pg_dir[0] = (uint32_t)entry_pg_table | 3;
  entry_pg_dir[MAP_PDE_IDX] = (uint32_t)entry_pg_dir | 3;
  entry_pg_dir[MAP_PG_DIR_PDE_IDX] = (uint32_t)reserve_for_map | 3;
  // virtual address 0~4M -> phy address 0~4M
  for (int i = 0; i < 1024; i++) {
    entry_pg_table[i] = (i * 4096) | 3;
  }
  reserve_for_map[0] = (uint32_t)entry_pg_dir | 3;
}
