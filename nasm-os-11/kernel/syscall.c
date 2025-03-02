#include "kernel/syscall.h"
#include "drivers/screen.h"
#include "libc/kprint.h"

void init_syscall()
{
  register_interrupt_handler(0x80, syscall_handler);
}

void syscall_handler(registers_t *r)
{
  LOGD("syscall_handler: %d %d %d %d\n", r->eax, r->ebx, r->ecx, r->edx);
  if (r->eax == 4) {
	if (r->ebx == 1) {
      kprint_k((char*)r->ecx, r->edx);
    }
  }
}
