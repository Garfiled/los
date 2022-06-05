#include "proc.h"
#include "page.h"
#include "../libc/kprint.h"
#include "../cpu/x86.h"
#include <stddef.h>


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
  p->pgdir = alloc_pte_for_proc();
  if (p->pgdir == NULL) {
    kprintf("alloc_pte_for_proc fail\n");
    return NULL;
  }
  p->stack = MAP_STACK_ADDR;
  //MEMSET(p->context, 0, sizeof *p->context);
  p->killed = 0;
  return p;
}

void test_proc()
{
  kprintf("test_proc\n");
  hang();
}

void scheduler(void)
{
  struct proc *p;
  hang();

  for(;;){
    // Loop over process table looking for process to run.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      //switchuvm(p);
      p->state = RUNNING;

      //swtch(&(c->scheduler), p->context);
      //switchkvm();
    }
  }
}
