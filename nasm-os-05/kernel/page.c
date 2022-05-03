#include "./page.h"
#include <stdint.h>

static inline void set_cr3(uint32_t cr3)
{
  __asm__ volatile("movl  %%eax, %%cr3" :: "a"(cr3));
}

static inline void open_mm_page()
{
  __asm__ volatile(
      "movl %cr0, %eax;"
      "orl  $0x80000000, %eax;"
      "movl    %eax, %cr0;"
  );
}

void install_page()
{
	//页目录开始于 [0x600000, 0x601000) ，大小为1024个（每个4字节）共4096字节
	unsigned int *page_dir = ((unsigned int *) PAGE_DIR);
	//页表1开始于0x600000 + 0x1000 = 0x601000
	unsigned int *page_table = ((unsigned int *) PAGE_TABLE);
	//用于内存地址计算
	unsigned int address = 0;
	//处理所有页目录
	//页表开始于 [0x601000, 0xa01000) ，大小为4M
	for (int i = 0; i < 1024; i++) {
		for (int j = 0; j < 1024; j++) {
			page_table[j] = address | 3;
			address += PAGE_ALIGN_SIZE;
		}
		page_dir[i] = ((unsigned int) page_table | 3);
		page_table += 1024;
	}

	//设置cr3寄存器为页目录的值
	set_cr3(PAGE_DIR);
	//开启内存分页
	open_mm_page();
}
