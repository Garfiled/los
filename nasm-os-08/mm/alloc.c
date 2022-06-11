#include "./alloc.h"
#include <stddef.h>
#include <stdint.h>
#include "../kernel/page.h"

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */

void* alloc_mm(int size)
{
  static uint32_t allocated = 20 * 1024 * 1024;
  //uint32_t address = allocated;
  void * ret = (void*)allocated;
  allocated += 4096;
  //uint32_t address = 16 * 1024 * 1024;
  return ret; 
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
