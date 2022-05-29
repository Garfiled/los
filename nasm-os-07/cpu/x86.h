#pragma once

#include <stdint.h>

#define UNUSED(x) (void)(x)

static inline uint32_t ebp()
{
  uint32_t ebp;
  __asm__ volatile("movl  %%ebp, %0" : "=a" (ebp) : );
  return ebp;
}

static inline uint32_t esp()
{
  uint32_t esp;
  __asm__ volatile("movl  %%ebp, %0" : "=a" (esp) : );
  return esp;
}

static inline void hang()
{
  while (1) {
    __asm__ volatile("hlt");
  }
}

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

