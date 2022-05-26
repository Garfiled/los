#pragma once

#include <stdint.h>

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

