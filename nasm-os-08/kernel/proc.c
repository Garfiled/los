#include "libc/kprint.h"
#include "cpu/x86.h"
#include "kernel/elf.h"
#include "fs/vfs.h"
#include "mm/alloc.h"
#include "kernel/lapic.h"
#include "kernel/proc.h"
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "libc/string.h"
#include <stddef.h>


struct {
  struct proc proc[NPROC];
} ptable;

int nextpid = 1;

struct proc *current_proc = NULL;

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

struct proc* alloc_proc(void *entry_func, const char* args)
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
  p->pgdir = (uint32_t)alloc_pte_for_proc(p->pid, args);
  if (p->pgdir == 0) {
    kprintf("alloc_pte_for_proc fail\n");
    return NULL;
  }
  p->stack = MAP_STACK_ADDR - 2048; // 3G - 2KB
  p->entry = (uint32_t)(entry_func);
  memset((char*)&p->context, 0, sizeof(p->context));
  p->context.eip = p->entry;
  p->context.esp = p->stack;
  p->context.ebp = p->stack;
  p->killed = 0;
  p->state = RUNNABLE;
  return p;
}

void release_proc(struct proc* p)
{
  kprintf("release_proc\n");
  p->state = ZOMBIE;
  p->pid = 0;
  p->stack = 0;
  p->entry = 0;
  p->killed = 0;

  free_pte_for_proc(p->pgdir);
  p->state = UNUSED;
}

void test_proc()
{
  kprintf("test_proc\n");
  kprintf("cr3:%x esp:%x\n",cr3(), esp());
}

int process_exec(const char* args)
{
  alloc_proc((void*)exec, args);
  return 0;
}

int exec(const char* args)
{
  char* path = (char*)args;

    // Read elf binary file.
  file_stat_t stat;
  if (stat_file(path, &stat) != 0) {
    kprintf("Command %s not found\n", path);
    return -1;
  }
  uint32_t size = stat.size;
  char* read_buffer = (char*)alloc_mm(size);
  if (read_file(path, read_buffer, 0, size) != (int32_t)(size)) {
    kprintf("Failed to load cmd %s\n", path);
    free_mm(read_buffer);
    return -1;
  }

  // kprintf("load_elf>>>>>>>>>>>>>>>>\n");
  // Load elf binary.
  uint32_t exec_entry;
  if (load_elf(read_buffer, &exec_entry)) {
    kprintf("load elf fail\n");
    free_mm(read_buffer);
    return -1;
  }

  kprintf("call_elf on entry:%x\n", exec_entry);
  asm volatile("movl %%eax, %%edx" :: "a"(exec_entry));
  asm volatile("call *%edx");
  free_mm(read_buffer);
  return 0;
}

void schedule(void)
{
  struct proc *p;
  struct proc* candi_p = NULL;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE) {
	  candi_p = p;
    } else if (p->state == ZOMBIE) {
      release_proc(p);
      current_proc = NULL;
    }
    if (candi_p) {
      candi_p->state = RUNNING;
      current_proc = candi_p;
      __asm__ volatile(
       "sti\n\t"
       "mov %0, %%cr3\n\t"
       "mov %1, %%esp\n\t"
       "mov %2, %%ebp\n\t"
       "jmp *%3"
         : : "r"(current_proc->pgdir),
             "r"(current_proc->context.esp),
             "r"(current_proc->context.ebp),
             "r"(current_proc->context.eip)
              );

	  }
  }
}

void exit(int status)
{
  UNUSED(status);
  current_proc->state = ZOMBIE;
  __asm__ volatile(
    "mov %0, %%cr3\n\t"
    "mov %1, %%esp\n\t"
	"jmp *%2"
    : : "r"(entry_pg_dir),
        "r"(PDE_SIZE),
        "r"(sched_loop)
  );
}
