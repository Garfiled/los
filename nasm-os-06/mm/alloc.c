#include <stddef.h>
#include <stdint.h>
#include "mm/alloc.h"
#include "kernel/page.h"

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */

void* alloc_mm(int size)
{
  UNUSED(size);
  static uint32_t allocated = 16 * 1024 * 1024;
  allocated += 4096;
  return (void*)allocated;
}

/*
 * free_mm : 释放内存
 *  - void* addr : 释放地址
 *  - int size : 释放大小
 * return : void
 */
void free_mm(void* addr)
{
  UNUSED(addr);
}
