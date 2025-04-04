// Per-CPU state

#pragma once

#include "../kernel/mp.h"
#include "../cpu/x86.h"

#define NPROC        4096

#define FL_IF           0x00000200

struct cpu {
  unsigned char apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  //struct taskstate ts;         // Used by x86 to find stack for interrupt
  //struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile unsigned int started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;
extern struct cpu* mycpu(void);

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  unsigned int edi;
  unsigned int esi;
  unsigned int ebx;
  unsigned int ebp;
  unsigned int eflags;
  unsigned int eip;
  unsigned int esp;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  int pid;                     // Process ID
  enum procstate state;        // Process state
  uint32_t pgdir;              // Page table
  uint32_t stack;              // Bottom of stack for this process
  struct context context;     // swtch() here to run process
  int killed;                  // If non-zero, have been killed
  char name[16];               // Process name (debugging)
  uint32_t entry;
  uint32_t stack_store;
};

static inline unsigned int readeflags(void)
{
  unsigned int eflags;
  asm volatile("pushfl; popl %0" : "=r" (eflags));
  return eflags;
}

struct proc* alloc_proc(void *entry_func, const char* args);
void test_proc();
void swtch(uint32_t, uint32_t);

extern struct proc *current_proc;
void schedule();
void exit(int status);
