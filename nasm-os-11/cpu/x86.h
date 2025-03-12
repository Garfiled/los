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
