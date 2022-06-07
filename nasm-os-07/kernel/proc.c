#include "proc.h"
#include "page.h"
#include "../libc/kprint.h"
#include "../cpu/x86.h"
#include <stddef.h>
#include "kernel.h"


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
  p->stack = MAP_STACK_ADDR;
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
      kprintf("scheduler:%x %x %x %d\n", p->pgdir, p, esp(), p->pid);
      //set_cr3(p->pgdir);
      set_cr3(entry_pg_dir2);
      kprintf("cr3:%x esp:%x\n",cr3(), esp());
      
      //hang();
      uint32_t esp1 = 4 * 1024 * 1024 - 4096;
      swtch(esp1, p->entry);
      // It should have changed its p->state before coming back.
      set_cr3(entry_pg_dir);
    }
  }
}
