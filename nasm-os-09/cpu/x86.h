#pragma once

#include <stdint.h>
#include <stddef.h>

#define UNUSED(x) (void)(x)

static inline uint32_t cs()
{
  uint32_t cs;
  __asm__ volatile("movl %%cs, %0" : "=a" (cs) : );
  return cs;
}

static inline uint32_t eip()
{
  uint32_t eip_val;

  // 通过 CALL-POP 方式获取当前 EIP
  __asm__ volatile (
      "call 1f\n\t"       // 调用下一条指令（标签 1）
      "1:\n\t"            // 定义标签 1
      "pop %0"            // 将返回地址（即标签1的地址）弹出到变量
      : "=r" (eip_val)
  );
  return eip_val;
}

static inline uint32_t ebp()
{
  uint32_t ebp;
  __asm__ volatile("movl %%ebp, %0" : "=a" (ebp) : );
  return ebp;
}

static inline uint32_t esp()
{
  uint32_t esp;
  __asm__ volatile("movl %%esp, %0" : "=a" (esp) : );
  return esp;
}

static inline uint32_t edx()
{
  uint32_t edx;
  __asm__ volatile("movl %%edx, %0" : "=a" (edx) : );
  return edx;
}

static inline void hang()
{
  while (1) {
    __asm__ volatile("hlt");
  }
}

static inline void set_cr3(uint32_t val)
{
  asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

static inline uint32_t cr3()
{
  uint32_t cr3;
  __asm__ volatile("movl %%cr3, %0" : "=a" (cr3) : );
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

static inline void close_mm_page()
{
  __asm__ volatile(
      "movl %cr0, %eax;"
      "and $~(1 << 31), %eax;"
      "movl %eax, %cr0;"
  );
}
