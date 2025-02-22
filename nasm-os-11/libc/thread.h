#pragma once

#include <stdint.h>

#define KERNEL_STACK_SIZE  8192

struct task_struct {
  uint32_t kernel_esp;
  uint32_t kernel_stack;

  uint32_t id;
  char name[32];

  // ...
};
typedef struct task_struct tcb_t;
typedef void (*thread_func)(void*);

tcb_t *init_thread(char* name, thread_func function, uint32_t prio, uint8_t is_user_thread);

