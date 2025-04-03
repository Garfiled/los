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

static inline int32_t atomic_exchange(int32_t* ptr, int32_t new_val)
{
  int32_t old_val;
  __asm__ __volatile__ (
      "xchgl %0, %1"        // xchgl 指令交换 %0 和 %1
      : "=r" (old_val),     // 输出操作数：old_val 接收旧值
        "+m" (*ptr)         // 输入输出操作数：内存地址 *ptr
      : "0" (new_val)       // 输入操作数：将 new_val 绑定到 %0（同 old_val）
      : "memory"            // 破坏列表：通知编译器内存被修改
  );
  return old_val;
}
