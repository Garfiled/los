#include "kernel/syscall.h"
#include "libc/kprint.h"


void init_syscall()
{
  register_interrupt_handler(0x80, syscall_handler);  
}

void syscall_handler(registers_t *r)
{
  kprintf("syscall_handler\n");
  if (r->eax == 4) {
    kprint_k(r->ecx, r->edx);
  }
}
