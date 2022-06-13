#include "mm/alloc.h"
#include "kernel/page.h"
#include <stddef.h>
#include <stdint.h>

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */

static uint32_t allocated = 20 * 1024 * 1024;

void* alloc_mm(int size)
{
  uint32_t addr = allocated;
  allocated += size;
  kprintf("alloc_mm allocated=%x size=%x %x\n", allocated, size, addr);
  return addr; 
}

void* alloc_mm_align(int size)
{
  if (allocated % 4096 != 0) {
    allocated = (allocated / 4096 + 1) * 4096;
  }
  if (size % 4096 != 0) {
    size = (size / 4096 + 1) * 4096;
  }
  uint32_t addr = allocated;
  allocated += size;
  kprintf("alloc_mm_align allocated=%x size=%x %x\n", allocated, size, addr);
  return addr; 
}
/*
 * free_mm : 释放内存
 *  - void* addr : 释放地址
 *  - int size : 释放大小
 * return : void
 */
void free_mm(void* addr)
{
}
