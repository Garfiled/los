#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ALIGN_MEM_SIZE 4096

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */
void* alloc_mm(int size);
void* alloc_mm_align(int size);

/*
 * free_mm : 释放内存
 *  - void* addr : 释放地址
 *  - int size : 释放大小
 * return : void
 */
void free_mm(void* addr);

struct mm_desc
{
  bool free_;
  int32_t len_;
  struct mm_desc *prev_;
};

