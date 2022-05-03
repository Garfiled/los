#pragma once

#include <stdint.h>

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

//内存页大小4096B
#define MM_PAGE_SIZE		(4 * 1024)
//内存总页数
#define MAP_SIZE			(0x20000)	//(MEM_SIZE * 1024 * 1024 / MM_PAGE_SIZE)
#define MAP_SIZE_LOGIC		(0x100000)	//(MEM_SIZE_LOGIC * 1024 * 1024 / MM_PAGE_SIZE)


/*
 * MMAP所在内存地址为 [0x100000, 0x200000)
 * MAP使用情况内存地址为 [0x200000, 0x600000)
 * Kernel所在页目录及页表地址 [0x600000, 0xa01000)
 * 从0x1000000以下为0x1000个内存页
 */
#define MMAP_USED_SIZE		(0x1000)

//map起始地址
#define MMAP				(0x100000)
//map_process起始地址
#define MMAP_PRO			(0x200000)

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

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */
void* alloc_mm(int size);

/*
 * free_mm : 释放内存
 *  - void* addr : 释放地址
 *  - int size : 释放大小
 * return : void
 */
void free_mm(void* addr);

uint8_t mmap_status(unsigned int page_no);

unsigned int map_process_id(unsigned int page_no);

void set_mmap_status(unsigned int page_no, uint8_t status);

void set_map_process_id(unsigned int page_no, unsigned int pid);

void free_page_by_pid(unsigned int pid);
