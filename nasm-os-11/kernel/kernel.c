#include "cpu/isr.h"
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "libc/string.h"
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
#include "libc/kprint.h"

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000
#define START_CORE_SIMPLE_PG 0x400000

char hd_num = 0;
struct HD hd[HD_NUM];

void kernel_main()
{
  kprintf("I am in kernel! ebp=%x esp=%x\n", ebp(), esp());
  isr_install();
  irq_install();

  hd_setup((char*)(SYSTEM_PARAM_ADDR));

  init_syscall();

  mpinit();

  //lapicinit();

  // phy memory mgr
  install_alloc();

  // 初始化processor启动过渡页表
  init_entry_page();

  // start other processor
  // startothers();

  set_cr3((uint32_t)entry_pg_dir);

  open_mm_page();

  register_interrupt_handler(14, page_fault_handler);

  // 初始化文件系统
  init_file_system();

  kprintf("create hello process>>>\n");
  process_exec("hello", 0, NULL);

  kprintf("start schedule process>>>\n");
  scheduler();
}

void* hd_setup(char* addr)
{
  hd_num = *addr;
  kprintf("hd_setup hd_num:%d\n", hd_num);
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

void print_hd()
{
  kprint("hd cyl head sector sector_bytes nsectors\n");
  for (int i = 0; i < hd_num; i++) {
    kprintf("%d %d %d %d %d %d\n", i, hd[i].cyl, hd[i].head, hd[i].sector, hd[i].sector_bytes, hd[i].nsectors);
  }
}

void print_mem(uint32_t addr)
{
  kprintf("mem:%x\n", *((uint32_t*)addr));
}

void cmd_read_hd(bool is_master_device, int start_sector)
{
  void* buf = alloc_mm(512);
  if (buf != NULL) {
    hd_rw(is_master_device, start_sector, HD_READ, 1, buf);
  } else {
    kprint("alloc mm fail");
  }
}

void cmd_write_hd(bool is_master_device, int start_sector, char *buf)
{
  kprintf("writehd:%d %d %s\n", is_master_device, start_sector, buf);
  void* mem_buf = alloc_mm(512);
  if (mem_buf != NULL) {
    memcpy(mem_buf, buf, strlen(buf));
    hd_rw(is_master_device, start_sector, HD_WRITE, 1, mem_buf);
  } else {
    kprint("alloc mm fail");
  }
}

void split(char *input, char delim, char **dest, int *count)
{
  *count = 0;
  if (input == NULL) {
    return;
  }
  int start = 0;
  int index = 0;

  for (int i = 0; ; i++) {
    if (input[i] == delim) {
      char *buf = (char*)alloc_mm(i - start + 1);
      //kprintf("addr1:%x %d\n", buf, i - start + 1);
      memcpy(buf, input + start, i - start);
      buf[i-start]='\0';
      dest[index] = buf;
      start = i + 1;
      index++;
      *count = *count + 1;
    } else if (input[i] == '\0') {
      if (i > 0) {
        char *buf = (char*)alloc_mm(i - start + 1);
        //kprintf("addr2:%x %d\n", buf, i - start + 1);
        memcpy(buf, input + start, i - start);
        buf[i-start]='\0';
        dest[index] = buf;
        start = i + 1;
        index++;
        *count = *count + 1;
      }
      break;
    }
  }
}

void user_input(char *input) {
  char *cmd[8] = {0};
  int cmd_num = 0;
  split(input, ' ', cmd, &cmd_num);
  //kprintf("cmd:%s cmd_num:%d\n", cmd[0], cmd_num);
  if (cmd_num == 0) {
    // do nothing
  } else if (strcmp(cmd[0], "exit") == 0) {
    kprint("Stopping the CPU. Bye!\n");
    asm volatile("hlt");
  } else if (strcmp(cmd[0], "lshd")==0) {
    print_hd();
  } else if (strcmp(cmd[0], "readhd")==0) {
    cmd_read_hd(atoi(cmd[1]), atoi(cmd[2]));
  } else if (strcmp(cmd[0], "checkhd")==0) {
    check_hd_status();
  } else if (strcmp(cmd[0], "resethd")==0) {
    reset_hd_controller();
  } else if (strcmp(cmd[0], "readmem")==0) {
    print_mem(atoi(cmd[1]));
  } else if (strcmp(cmd[0], "writehd") == 0) {
    cmd_write_hd(atoi(cmd[1]), atoi(cmd[2]), cmd[3]);
  } else if (strcmp(cmd[0], "cls") == 0) {
    clear_screen();
  } else if (strcmp(cmd[0], "malloc") == 0) {
    void *alloc_buf = alloc_mm(1);
    kprintf("-- alloc_buf=%x\n", alloc_buf);
  } else if (strcmp(cmd[0], "writemem") == 0) {
    memcpy((char*)(atoi(cmd[1])), cmd[2], strlen(cmd[2]));
  } else if (strcmp(cmd[0], "ls") == 0) {
	list_dir("/");
  } else if (strcmp(cmd[0], "write") == 0) {
	write_file(cmd[1], cmd[2], atoi(cmd[3]), atoi(cmd[4]));
  } else if (strcmp(cmd[0], "read") == 0) {
	uint32_t offset = atoi(cmd[2]);
	uint32_t length = atoi(cmd[3]);
	char * buf = (char*)alloc_mm(length);
	read_file(cmd[1], buf, offset, length);
	kprint_k(buf, length);
	kprintf("\n");
  }
  kprint("los> ");
}

// Start the non-boot (AP) processors.
void startothers(void)
{
  kprintf("startothers: %d\n", ncpu);
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

    lapicstartap(c->apicid, (unsigned int)code);

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
  kprintf("entry_pg: %x %x\n", entry_pg_dir, entry_pg_table);
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
