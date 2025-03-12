#pragma once

#include "cpu/isr.h"
#include <stdint.h>


#define PAGE_ALIGN_SIZE (4096)
#define PDE_SIZE 4 * 1024 * 1024

// 4G - 4M
#define MAP_PAGE_TABLE_START (uint32_t)0xFFC00000
// 4G - 4KB
#define MAP_PAGE_TABLE_PG_DIR (uint32_t)(0xFFFFF000)
// 4G - 8KB
#define MAP_PAGE_TABLE_UNUSE (uint32_t)(0xFFFFE000)
// 3G
#define MAP_STACK_ADDR 0xC0000000

#define MAP_UNUSED_IDX 1022
#define MAP_PDE_IDX 1023
#define MAP_STACK_PDE_IDX 768

#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PS          0x080   // Page Size


//内存总大小以M为单位
#define MEM_SIZE			(4096)
#define MEM_SIZE_LOGIC		(4096)

//位图状态标识
//#define MM_SWAP_TYPE_CAN	(1)
//#define MM_SWAP_TYPE_NO		(0)
//未使用
#define MM_FREE				(0x0 << 0)
//已使用
#define MM_USED				(0x1 << 0)
//是否可交换
#define MM_NO_SWAP			(0x0 << 1)
#define MM_CAN_SWAP			(0x1 << 1)
//动态分配
#define MM_NO_DYNAMIC		(0x0 << 2)
#define MM_DYNAMIC			(0x1 << 2)

//内存总页数
#define MAP_SIZE			    (0x20000)	//(MEM_SIZE * 1024 * 1024 / PAGE_ALIGN_SIZE)
#define MAP_SIZE_LOGIC		(0x100000)	//(MEM_SIZE_LOGIC * 1024 * 1024 / PAGE_ALIGN_SIZE)


/*
 * MMAP所在内存地址为 [0x100000, 0x200000)
 * MAP使用情况内存地址为 [0x200000, 0x600000)
 * Kernel所在页目录及页表地址 [0x600000, 0xa01000)
 * 从0x1000000以下为0x1000个内存页
 */
#define MMAP_USED_SIZE		(0x1800)

//map起始地址
#define MMAP				(0x100000)
//map_process起始地址
#define MMAP_PRO			(0x200000)

typedef struct isr_params {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_num;
  uint32_t err_code;
  uint32_t eip, cs, eflags, user_esp, user_ss;
} isr_params_t;

void* alloc_pte_for_proc(uint32_t pid);
void free_pte_for_proc(uint32_t phy_addr);
void map_pte(uint32_t addr);
void page_fault_handler(registers_t *r);
/*
 * install_alloc : 安装申请内存位图
 * return : void
 */
void install_alloc();

void install_used_map();

/*
 * alloc_page : 申请内存页，每页为4KB大小
 *  - int count : 页数
 * return : void*返回申请地址，NULL代表申请失败
 */
void* alloc_page(unsigned int process_id, unsigned int count, unsigned int can_swap, unsigned int is_dynamic);

/*
 * alloc_page : 释放内存页，每页为4KB大小
 *  - void *addr : 释放地址
 *  - int count : 释放页数
 * return : void
 */
void free_page(void *addr, unsigned int count);

