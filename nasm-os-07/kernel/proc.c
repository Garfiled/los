#include "proc.h"
#include "page.h"
#include "../libc/kprint.h"
#include "../cpu/x86.h"
#include <stddef.h>
#include "kernel.h"
#include "../libc/string.h"


struct {
  struct proc proc[NPROC];
} ptable;

int nextpid = 1;

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu* mycpu(void)
{
  int apicid, i;

  /*
  if(readeflags() & FL_IF) {
    kprintf("mycpu called with interrupts enabled!\n");
    return NULL;
  }
  */
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  kprintf("unknown apicid %d!\n", apicid);
  hang();
  return NULL;
}

struct proc* alloc_proc()
{
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  return NULL;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  // pg dir
  p->pgdir = (uint32_t)alloc_pte_for_proc();
  if (p->pgdir == 0) {
    kprintf("alloc_pte_for_proc fail\n");
    return NULL;
  }
  // stack 1G+3K
  p->stack = MAP_STACK_ADDR + 3 * 1024;
  p->entry = test_proc;
  //MEMSET(p->context, 0, sizeof *p->context);
  p->killed = 0;
  
  p->state = RUNNABLE;
  return p;
}

void test_proc()
{
  kprintf("test_proc\n");
  kprintf("cr3:%x esp:%x\n",cr3(), esp());
  hang();
}

void scheduler(void)
{
  struct proc *p;

  for(;;){
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
      
      p->state = RUNNING;
      kprintf("scheduler> pid:%d pgdir:%x stack:%x esp:%x\n", p->pid, p->pgdir, p->stack, esp());
      set_cr3(p->pgdir);
      kprintf("cr3:%x esp:%x\n",cr3(), esp());
      
      swtch(p->stack, p->entry);
      // It should have changed its p->state before coming back.
      set_cr3(entry_pg_dir);
    }
  }
}
