#include <stdint.h>
#include <stddef.h>
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "libc/kprint.h"
#include "kernel/page.h"
#include "cpu/x86.h"
#include "libc/string.h"

void* alloc_pte_for_proc()
{
  kprintf("alloc_pte_for_proc start\n");
  void *phy_addr = alloc_page(1, 5, 0, 0);
  if (phy_addr == NULL) {
    return NULL;
  }
  uint32_t pg_dir_phy_addr = (uint32_t)phy_addr;
  uint32_t first_pde_phy_addr = (uint32_t)phy_addr + 1 * PAGE_ALIGN_SIZE;
  uint32_t stack_pde_phy_addr = (uint32_t)phy_addr + 2 * PAGE_ALIGN_SIZE;
  uint32_t stack_phy_addr = (uint32_t)phy_addr + 3 * PAGE_ALIGN_SIZE;
  uint32_t unused_phy_addr = (uint32_t)phy_addr + 4 * PAGE_ALIGN_SIZE;

  // 这些页表只有物理地址, 要想维护必须先建立映射关系
  uint32_t can_map_address = MAP_PAGE_TABLE_UNUSE;
  uint32_t* map_address = (uint32_t*)can_map_address;

  uint32_t* first_page_table = (uint32_t*)MAP_PAGE_TABLE_START;
  //kprintf("debug0: %x %x\n", first_page_table, *first_page_table);
  // 2GB ~ 2GB+4M 映射所有二级页表，如何做到的呢？ 只需要pg_dir[512] = pg_dir_phy | 3
  uint32_t *page_table = (uint32_t*)(MAP_PAGE_TABLE_START + 4 * (can_map_address / PAGE_ALIGN_SIZE));
  //kprintf("debug:%x %x\n", page_table, *page_table);
  // 找到当前未做映射的page_table， TODO 处理并发问题
  if (*page_table != 0) {
    // 已被占用
    return NULL;
  }

  // 设置页目录
  *page_table = pg_dir_phy_addr | 3;
  MEMSET(map_address, 0 , 4096);
  map_address[0] = first_pde_phy_addr | 3; // 第一个二级页表
  map_address[MAP_STACK_PDE_IDX] = stack_pde_phy_addr | 3; // 虚拟地址空间1G处
  map_address[MAP_PDE_IDX] = pg_dir_phy_addr | 3; // 虚拟地址空间2G处，这个操作有点奇葩
  map_address[MAP_PG_DIR_PDE_IDX] = unused_phy_addr | 3; // 虚拟地址空间2G+4M处，这个操作有点奇葩

  asm("invlpg (%0)" : :  "r"(map_address));
  // 0~4M 映射0~4M
  *page_table = first_pde_phy_addr | 3;
  MEMSET(map_address, 0 , 4096);
  for (int i = 0; i < 1024; i++) {
    map_address[i] = (i *  PAGE_ALIGN_SIZE) | 3;
  }

  asm("invlpg (%0)" : :  "r"(map_address));
  // 2G+4M～2G+4M+4K的虚拟地址空间映射到页目录
  *page_table = unused_phy_addr | 3;
  MEMSET(map_address, 0 , 4096);
  map_address[0] = pg_dir_phy_addr | 3;

  // 栈地址
  asm("invlpg (%0)" : :  "r"(map_address));
  *page_table = stack_pde_phy_addr | 3;
  // 虚拟地址空间1G处
  MEMSET(map_address, 0 , 4096);
  map_address[0] = (uint32_t)stack_phy_addr | 3;

  // 设置回去
  asm("invlpg (%0)" : :  "r"(map_address));
  *page_table = 0;

  //kprintf("debug>%x %d %x\n", esp(), ebp(), pg_dir_phy_addr);

  return (void*)pg_dir_phy_addr;
}

void free_pte_for_proc(uint32_t phy_addr)
{
  uint32_t can_map_address = MAP_PAGE_TABLE_UNUSE;
  uint32_t* map_address = (uint32_t*)can_map_address;

  uint32_t *page_table = (uint32_t*)(MAP_PAGE_TABLE_START + 4 * (can_map_address / PAGE_ALIGN_SIZE));
  // 找到当前未做映射的page_table， TODO 处理并发问题
  if (*page_table != 0) {
    // 已被占用
    return;
  }
  for (int i=1; i < 1024; i++) {
    asm("invlpg (%0)" : :  "r"(map_address));
    *page_table = phy_addr | 3;
    uint32_t pde_addr = map_address[i];
    if (pde_addr == 0) {
      continue;
    }
    asm("invlpg (%0)" : :  "r"(map_address));
    *page_table = pde_addr;
    for (int j=0; j < 1024; j++) {
      uint32_t pte_addr = map_address[j];
      if (pte_addr > 0) {
         free_page((void*)(pte_addr & (~0x3)), 1);
      }
    }
  }
  asm("invlpg (%0)" : :  "r"(map_address));
  *page_table = 0;
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

/*
 * install_alloc : 安装申请内存位图
 * return : void
 */
// 物理内存
void install_alloc()
{
  //位图所在的固定内存区域为 [0x100000, 0x200000)
  // 每个字节映射4k的话，一共可以映射4GB的内存空间
  mmap = (uint8_t *) MMAP;
  for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
    //16M以下均为已使用
    if (i < MMAP_USED_SIZE) {
      //设定内核所占用的1MB内存为已使用
      mmap[i] = (MM_USED | MM_NO_SWAP);
    } else {
      //剩下的内存为未使用
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
  for (uint32_t i = MMAP_USED_SIZE; i < MAP_SIZE; i++) {
    //如果找到空闲页
    if ((mmap[i] & 0x1) == MM_FREE) {
      //设置可分配内存起始编号
      if (start_with == 0) {
        ret = (void*) (i * PAGE_ALIGN_SIZE);
        start_with = i;
       }
       num++;
    }
    //如果没有找到空闲页
    else {
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
  //kprintf("alloc_page: addr=%x count=%d\n", ret, count);
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
  for (uint32_t i = 0; i < count; i++) {
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
  // 虚拟地址空间2G + 4M映射了页目录
  uint32_t *page_dir_addr = (uint32_t *)MAP_PAGE_TABLE_PG_DIR;

  //asm("invlpg (%0)" : :  "r"(page_dir_addr));
  // 高10位存放的是dir offset
  uint32_t page_dir_offset = address >> 22;
  // 中间的10位存放的是page_table offset
  uint32_t page_table_offset = (address >> 12) & 0x3FF;
  //kprintf("find_page_table:%x %x\n", MAP_PAGE_TABLE_PG_DIR, page_dir_addr);
  //kprintf("find_page_table: %d %d %x\n", page_dir_offset, page_table_offset, address);
  /*
  if (open_debug) {
    kprintf("debug>>%x %x\n", esp(), cr3());
    pp = cr3();
    kprintf("debug>>>:%x %x\n", pp, cr3());
    asm volatile(
      "movl %cr0, %eax;"
      "and $~(1 << 31), %eax;"
      "movl %eax, %cr0;"
  );
    asm volatile("movl %%eax, %%esp" :: "a"(0x400000));
    kprintf("close_mm_page:%x\n", pp);
    kprintf("pde entry>>>%x %x %x %x %x\n",pp, pp[0], pp[1], pp[512],pp[513]);
    ppp = pp[513] - 0x3;
    kprintf("entry>>>>%x %x\n", ppp, ppp[0]);
    hang();
  }
  */
  uint32_t page_table_base = page_dir_addr[page_dir_offset];
  // 注意这里的page_table_base非0的话也是物理地址，不能直接使用
  if (page_table_base == 0) {
    void *new_phy_page = alloc_page(1, 1, 0, 0);
    page_dir_addr[page_dir_offset] = (uint32_t)new_phy_page | 3;
  }
  uint32_t *page_table = (uint32_t*)(MAP_PAGE_TABLE_START + 4 * (page_dir_offset * 1024 + page_table_offset));
  //kprintf("--find_page_table:%d %d %x %x %x\n", page_dir_offset, page_table_offset, page_dir_addr[page_dir_offset], page_table_base, page_table);
  return page_table;
}

// 映射物理内存
void map_pte(uint32_t addr)
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
  //kprintf("--map_pte succ %x %x\n", addr, phy_page);
}

// 处理缺页中断
void page_fault_handler(registers_t *r)
{
  uint32_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
  //kprintf("--page fault %x!\n", faulting_address);

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
  UNUSED(present);
  UNUSED(rw);
  UNUSED(user_mode);
  UNUSED(reserved);
  UNUSED(id);

  map_pte(faulting_address);
}
