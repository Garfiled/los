#include "cpu/x86.h"
#include "libc/kprint.h"
#include "fs/file.h"
#include "mm/alloc.h"
#include "kernel/proc.h"
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "kernel/elf.h"
#include "kernel/lapic.h"
#include "fs/vfs.h"

struct {
  struct proc proc[NPROC];
} ptable;

struct proc *current_proc = NULL;
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

struct proc* alloc_proc(void *entry_func)
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
    kprintf("failed to alloc_pte_for_proc\n");
    return NULL;
  }
  p->stack = MAP_STACK_ADDR + 3 * 1024;
  p->entry = (uint32_t)(entry_func);
  p->context.eip = p->entry;
  //MEMSET(p->context, 0, sizeof *p->context);
  p->killed = 0;
  p->state = RUNNABLE;
  return p;
}

void release_proc(struct proc* p)
{
  LOGI("release_proc:%d\n", p->pid);
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

void schedule(void)
{
  for(;;) {
	struct proc* candi_p = NULL;
	int candi_score = 0;
    for(struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE) {
	    if (candi_p == NULL || candi_p->time_quantum < candi_score) {
		  candi_p = p;
		  candi_score = candi_p->time_quantum;
		}
	  }
	}
	if (candi_p == NULL) {
	  current_proc = NULL;
	} else if (current_proc != candi_p) {
      candi_p->state = RUNNING;
	  candi_p->time_quantum = TIME_QUANTUM;
      set_cr3(candi_p->pgdir);
	  current_proc = candi_p;
	  // 上下文切换
	  swtch(candi_p->stack, candi_p->context.eip);
      // 当切换回来时恢复原页表
      set_cr3((uint32_t)(entry_pg_dir));
	  release_proc(candi_p);
	}
  }
}

int process_exec(const char *path, int argc, char *argv[])
{
  // kprintf("process_exec>\n");
  UNUSED(path);
  UNUSED(argc);
  UNUSED(argv);
  alloc_proc((void*)exec);
  return 0;
}

int exec(char *path, int argc, char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);
  path = (char*)"hello";

    // Read elf binary file.
  file_stat_t stat;
  if (stat_file(path, &stat) != 0) {
    kprintf("Command %s not found\n", path);
    return -1;
  }
  uint32_t size = stat.size;
  LOGD("read_file: name=%s size=%d\n", path, stat.size);
  char* read_buffer = (char*)alloc_mm(size);
  if (read_file(path, read_buffer, 0, size) != (int32_t)(size)) {
    LOGE("Failed to load cmd %s\n", path);
    free_mm(read_buffer);
    return -1;
  }

  // kprintf("load_elf>>>>>>>>>>>>>>>>\n");
  // Load elf binary.
  uint32_t exec_entry;
  if (load_elf(read_buffer, &exec_entry)) {
    LOGE("load elf fail\n");
    free_mm(read_buffer);
    return -1;
  }

  LOGD("call_elf on entry:%x\n", exec_entry);
  asm volatile("movl %%eax, %%edx" :: "a"(exec_entry));
  asm volatile("call %edx");
  free_mm(read_buffer);
  return 0;
}
