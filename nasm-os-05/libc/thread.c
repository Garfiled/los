#include <stddef.h>
#include "./thread.h"
#include "../mm/alloc.h"
#include "../libc/kprint.h"
#include "../kernel/page.h"

tcb_t* init_thread(char* name,
                   thread_func function,
                   uint32_t priority,
                   uint8_t is_user_thread) {
  tcb_t *thread = (tcb_t*)alloc_mm(sizeof(struct task_struct));
  if (thread == NULL) {
    kprint("init thread alloc fail");
    return NULL;
  }
  uint32_t kernel_stack = (uint32_t)alloc_page(1, KERNEL_STACK_SIZE/PAGE_ALIGN_SIZE, 0, 0);
  if (kernel_stack == 0) {
    kprint("init thread alloc fail");
    return NULL;
  }
  //memset((void*)kernel_stack, 0, KERNEL_STACK_SIZE);
  thread->kernel_stack = kernel_stack;
  return thread;  
}
