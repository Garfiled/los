#include <stddef.h>
#include "libc/thread.h"
#include "mm/alloc.h"
#include "libc/kprint.h"
#include "kernel/page.h"
#include "libc/function.h"

tcb_t* init_thread(char* name,
                   thread_func function,
                   uint32_t priority,
                   uint8_t is_user_thread)
{
  UNUSED(name);
  UNUSED(priority);
  UNUSED(function);
  UNUSED(is_user_thread);
  tcb_t *thread = (tcb_t*)alloc_mm(sizeof(struct task_struct));
  if (thread == NULL) {
    kprintf("init thread alloc fail\n");
    return NULL;
  }
  uint32_t kernel_stack = (uint32_t)alloc_page(1, KERNEL_STACK_SIZE/PAGE_ALIGN_SIZE, 0, 0);
  if (kernel_stack == 0) {
    kprintf("init thread alloc fail\n");
    return NULL;
  }
  //memset((void*)kernel_stack, 0, KERNEL_STACK_SIZE);
  thread->kernel_stack = kernel_stack;
  return thread;
}
