#include "cpu/isr.h"
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "libc/kprint.h"
#include "mm/alloc.h"
#include "kernel/kernel.h"
#include "kernel/page.h"

#include <stdbool.h>
#include <stdint.h>

extern uint32_t tick;

#define HD_NUM 2
#define SYSTEM_PARAM_ADDR 0x9000

char hd_num = 0;
struct HD hd[HD_NUM];

void kernel_main()
{
  kprint("I am in kernel!\n");
  isr_install();
  irq_install();

  hd_setup((void*)(SYSTEM_PARAM_ADDR));

  // install virtual memory mangement
  install_alloc();
  install_page();
  kprint("los> ");
}

void* hd_setup(void* addr)
{
  hd_num = *((char*)addr);
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

void read_hd(bool is_master_device, int start_sector)
{
  void* buf = alloc_mm(512);
  if (buf != NULL) {
    hd_rw(is_master_device, start_sector, HD_READ, 1, buf);
  } else {
    kprint("alloc mm fail");
  }
}

void write_hd(bool is_master_device, int start_sector, char *buf)
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

void user_input(char *input)
{
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
    read_hd(atoi(cmd[1]), atoi(cmd[2]));
  } else if (strcmp(cmd[0], "checkhd")==0) {
    check_hd_status();
  } else if (strcmp(cmd[0], "resethd")==0) {
    reset_hd_controller();
  } else if (strcmp(cmd[0], "readmem")==0) {
    print_mem(atoi(cmd[1]));
  } else if (strcmp(cmd[0], "writehd") == 0) {
    write_hd(atoi(cmd[1]), atoi(cmd[2]), cmd[3]);
  } else if (strcmp(cmd[0], "cls") == 0) {
    clear_screen();
  } else if (strcmp(cmd[0], "malloc") == 0) {
    void *alloc_buf = alloc_mm(1);
    kprintf("-- alloc_buf=%x\n", alloc_buf);
  } else if (strcmp(cmd[0], "writemem") == 0) {
    memcpy((char*)(atoi(cmd[1])), cmd[2], strlen(cmd[2]));
  }
  kprint("los> ");
}
