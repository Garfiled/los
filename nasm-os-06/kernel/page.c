#include <stdint.h>
#include <stddef.h>
#include "../drivers/screen.h"
#include "../libc/kprint.h"
#include "./page.h"

static inline void set_cr3(uint32_t cr3)
{
  asm volatile("movl %%eax, %%cr3" :: "a"(cr3));
}

static inline uint32_t cr3()
{
  uint32_t cr3;
  __asm__ volatile("movl  %%cr3, %0" : "=a" (cr3) : );
  return cr3;
}

static inline void open_mm_page()
{
  __asm__ volatile(
      "movl %cr0, %eax;"
      "orl $0x80000000, %eax;"
      "movl %eax, %cr0;"
  );
}

// 页表设置
void install_page()
{
  kprintf("install page\n");
  // 分配物理内存
  // TODO 一次性分配出来的话内存占用较多，可以只创建一级页表
  void *phy_addr = alloc_page(1, 1024 + 1, 0, 0);
  if (phy_addr == NULL) {
    kprintf("install_page alloc fail!\n");
    return;
  }
	//大小为1024个（每个4字节）共4096字节
	uint32_t *page_dir = (uint32_t *) phy_addr;
	uint32_t *first_page_table = (uint32_t *) ((char*)page_dir + 4096);
  uint32_t *page_table = first_page_table;
	//用于虚拟内存地址计算
	uint32_t address = 0;
	//处理所有页目录
	for (int i = 0; i < 1024; i++) {
		for (int j = 0; j < 1024; j++) {
      // 前4MB
      if (i <= 0) {
			  page_table[j] = address | 3;
      } else {
			  page_table[j] = 0;
      }
			address += PAGE_ALIGN_SIZE;
		}
		page_dir[i] = ((uint32_t) page_table | 3);
		page_table += 1024;
	}
  // 将页表占用的内存映射好
  for (int i = 0; i < 1025; i++) {
    int addr = (uint32_t)phy_addr + (i * 4096);
    first_page_table[i + 1024] = addr | 3;
  }

	//设置cr3寄存器为页目录的值
	set_cr3((uint32_t)phy_addr);

  void *stack_phy_addr = alloc_page(1, 1, 0, 0);
  first_page_table[262144-1] = (uint32_t)stack_phy_addr | 3;
	//开启内存分页
	open_mm_page();

  static int reg_once = 0;
  if (reg_once == 0) {
    reg_once = 1;
    register_interrupt_handler(14, page_fault_handler);
  }
  //while (1) {
  //  __asm__ volatile("hlt");
 // }
}

//内存使用位图
uint8_t *mmap = NULL;

/*
 * 使用map_process存入每一个内存页面所被程序使用情况
 * map_process中存放了使用这个内存页的任务ID所在的
 * 固定内存区域为 [0x200000, 0x600000) 占用4M
 * 对于内核程序所使用的内存页面map_process被设置为0
 * 其它程序所使用的内存页面被设置为其任务ID
 */
uint32_t *map_process = NULL;
uint32_t find_index = MMAP_USED_SIZE;

/*
 * install_alloc : 安装申请内存位图
 * return : void
 */
// 物理内存
void install_alloc()
{
  //位图所在的固定内存区域为 [0x100000, 0x200000)
  mmap = (uint8_t *) MMAP;
  for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
    //16M以下均为已使用
    if (i < MMAP_USED_SIZE) {
      //设定内核所占用的1MB内存为已使用
      mmap[i] = (MM_USED | MM_NO_SWAP);
    }
    //剩下的内存为未使用
    else {
      mmap[i] = (MM_FREE | MM_CAN_SWAP);
    }
  }
}

//初始化内存页的使用者
void install_used_map()
{
	map_process = (uint32_t *) MMAP_PRO;
	/*
	 * 初始化map_process，因为map和map_process的大小永远都是一样的所以用MAP_SIZE
	 * map_process所占内存空间为 [0x200000, 0x600000)
	 */
	for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
		map_process[i] = 0;
	}
}

/*
 * alloc_page : 申请内存页，每页为4KB大小
 *  - int count : 页数
 * return : void*返回申请地址，NULL代表申请失败
 */
// 分配的是物理页面
void* alloc_page(uint32_t process_id, uint32_t count, uint32_t can_swap, uint32_t is_dynamic)
{
  UNUSED(is_dynamic);
  UNUSED(can_swap);
  //查找内存申请地址
  void *ret = NULL;
  //找到空闲内存页计数
  uint32_t num = 0;
  //开始编号
  uint32_t start_with = 0;
  //从未被分配内存页的地方开始查找
  for (uint32_t i = MMAP_USED_SIZE; i < MAP_SIZE; i++)
  {
    //如果找到空闲页
    if ((mmap[i] & 0x1) == MM_FREE) {
      //设置可分配内存起始编号
      if (start_with == 0) {
      	ret = (void*) (i * PAGE_ALIGN_SIZE);
      	start_with = i;
      }
      num++;
    } else {
      //如果没有找到空闲页
      //清空变量
      ret = NULL;
      num = 0;
      start_with = 0;
    }
    //找到了可分配内存页，并且找到了预期想要分配的数量
    if (start_with != 0 && num >= count) {
    	break;
    }
  }
  //设置map的各个内存页的状态为已使用
  for (uint32_t i = 0; i < count; i++) {
    mmap[start_with + i] = MM_USED | MM_CAN_SWAP; //(MM_USED | ((uint32_t) can_swap << 1) | ((uint32_t) is_dynamic << 2));
    map_process[start_with + i] = process_id;
  }
  kprintf("alloc_page: addr=%x count=%d\n", ret, count);
  //返回查找到内存地址
  return ret;
}

/*
 * alloc_page : 释放内存页，每页为4KB大小
 *  - void *addr : 释放地址
 *  - int count : 释放页数
 * return : void
 */
void free_page(void *addr, uint32_t count)
{
	//释放内存页
	for (uint32_t i = 0; i < count; i++)
	{
		//更新map中这些页的状态
		mmap[(uint32_t) (addr + (i * PAGE_ALIGN_SIZE)) / PAGE_ALIGN_SIZE] = (MM_FREE | MM_CAN_SWAP | MM_NO_DYNAMIC);
		map_process[i] = 0;
	}
}

void free_page_by_pid(uint32_t pid)
{
	for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
		if (map_process[i] == pid) {
			mmap[i] = (MM_FREE | MM_CAN_SWAP);
			map_process[i] = 0;
		}
	}
}

uint32_t *find_page_table(uint32_t address)
{
	uint32_t *page_dir = 0x400000;

  // 高10位存放的是dir offset
  uint32_t page_dir_offset = address >> 22;
  // 中间的10位存放的是page_table offset
  uint32_t page_table_offset = (address >> 12) & 0x3FF;
  kprintf("find_page_table: %x %d %d %x\n", page_dir, page_dir_offset, page_table_offset, address);
  uint32_t page_table_base = page_dir[page_dir_offset];
  // 去除低位的flags
  page_table_base = (page_table_base >> 12) << 12;
  uint32_t *page_table = (uint32_t*)page_table_base + page_table_offset;
  kprintf("--find_page_table:%d %d %x %x %x\n", page_dir_offset, page_table_offset, page_dir[page_dir_offset], page_table_base, page_table);
  //asm volatile("hlt");
  return page_table;
}

void bind_addr_phy_page(uint32_t addr)
{
  // to find page table
  uint32_t *page_table = find_page_table(addr);
  if (*page_table != 0x0) {
    kprintf("--page_table is not empty:%x %x\n", *page_table, addr);
    return;
  }
  void *phy_page = alloc_page(1, 1, 0, 0);
  if (phy_page == NULL) {
    kprintf("--alloc_page fail %x\n", addr);
    return;
  }
  *page_table = (uint32_t)phy_page | 3;
  kprintf("--bind_addr_phy_page succ %x %x\n", addr, phy_page);
}

void page_fault_handler(registers_t *r)
{
  uint32_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
  kprintf("--page fault %x!\n", faulting_address);

  // page not present?
  int present = r->err_code & 0x1;
  // write operation?
  int rw = r->err_code & 0x2;
  // processor was in user-mode?
  int user_mode = r->err_code & 0x4;
  // overwritten CPU-reserved bits of page entry?
  int reserved = r->err_code & 0x8;
  // caused by an instruction fetch?
  int id = r->err_code & 0x10;

  bind_addr_phy_page(faulting_address);
}
