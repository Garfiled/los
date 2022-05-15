// Per-CPU state

#pragma once

#include "../kernel/mp.h"


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
  unsigned int eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  unsigned int sz;                     // Size of process memory (bytes)
  //pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  //struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};



static inline unsigned int readeflags(void)
{
  unsigned int eflags;
  asm volatile("pushfl; popl %0" : "=r" (eflags));
  return eflags;
}


// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
