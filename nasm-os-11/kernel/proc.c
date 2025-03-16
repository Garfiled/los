#include "cpu/x86.h"
#include "libc/kprint.h"
#include "fs/file.h"
#include "mm/alloc.h"
#include "kernel/proc.h"
#include "kernel/page.h"
#include "kernel/kernel.h"
#include "libc/string.h"
#include "kernel/elf.h"
#include "kernel/lapic.h"
#include "fs/vfs.h"

struct {
  struct proc proc[NPROC];
} ptable;

struct proc *current_proc = NULL;
uint32_t nextpid = 1;

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

  LOGE("alloc_proc slot full!\n");
  return NULL;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  // pg dir
  p->pgdir = (uint32_t)alloc_pte_for_proc(p->pid, args);
  if (p->pgdir == 0) {
    LOGE("failed to alloc_pte_for_proc\n");
    return NULL;
  }
  p->stack = MAP_STACK_ADDR - 2048; // 3G - 2KB
  p->entry = (uint32_t)(entry_func);
  memset((char*)&p->context, 0, sizeof(p->context));
  p->context.eip = p->entry;
  p->context.esp = p->stack;
  p->context.ebp = p->stack;
  p->killed = 0;
  p->priority = TIME_QUANTUM;
  p->state = RUNNABLE;
  LOGI("alloc_proc proc:%x pid:%d entry:%x\n", p, p->pid, entry_func);
  return p;
}

void release_proc(struct proc* p)
{
  LOGI("release_proc pid:%d proc:%x esp:%x\n", p->pid, p, esp());
  p->state = ZOMBIE;
  p->pid = 0;
  p->stack = 0;
  p->entry = 0;
  p->killed = 0;
  p->priority = 0;
  p->entry = 0;

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
  struct proc* candi_p = NULL;
  for(struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE) {
      if (candi_p == NULL) {
  	    candi_p = p;
	  } else if (p->priority > candi_p->priority) {
  	    candi_p = p;
  	  } else if (p->priority == candi_p->priority && candi_p == current_proc && p != current_proc) {
		candi_p = p;
	  }
    } else if (p->state == ZOMBIE) {
	  release_proc(p);
	  current_proc = NULL;
	}
  }
  if (candi_p) {
    LOGD("schedule find candi_p:%x pid:%d priority:%d eip:%x esp:%x curr_p:%x curr_esp:%x\n", candi_p, candi_p->pid, candi_p->priority,
		  candi_p->context.eip, candi_p->context.esp, current_proc, esp());
  } else {
	static int32_t last_tick_time = 0;
    int32_t curr_tick = tick;
	if (curr_tick - last_tick_time / 1000 * 1000 > 1000) {
	  LOGD("schedule empty RUNNABLE proc eip:%x esp:%x %d\n", eip(), esp(), tick);
	  last_tick_time = curr_tick;
	}
  }
  struct proc* last_p = current_proc;
  if (candi_p) {
    candi_p->state = RUNNING;
    candi_p->priority = TIME_QUANTUM;
    current_proc = candi_p;
    if (last_p == NULL) {
	  // 调用完怎么再回到这里？
      __asm__ volatile(
	    "sti\n\t"
		"mov %0, %%cr3\n\t"
        "mov %1, %%esp\n\t"
        "mov %2, %%ebp\n\t"
        "jmp %3"
        : : "r"(current_proc->pgdir),
		    "r"(current_proc->context.esp),
		    "r"(current_proc->context.ebp),
		    "r"(current_proc->context.eip)
      );
	} else if (current_proc != last_p) {
      __asm__ volatile(
	    "sti\n\t"
		"mov %0, %%cr3\n\t"
        "mov %1, %%esp\n\t"
        "mov %2, %%ebp\n\t"
		"jmp %3"   // 跳转过去执行，执行完还能回到这里
        : : "r"(current_proc->pgdir),
		    "r"(current_proc->context.esp),
		    "r"(current_proc->context.ebp),
		    "r"(current_proc->context.eip)
      );
	}
  }
}

int process_exec(const char* args)
{
  alloc_proc((void*)exec, args);
  return 0;
}

int exec(const char* args)
{
  UNUSED(args);
  char* path = (char*)"hello";

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

void exit(int status)
{
  UNUSED(status);
  current_proc->state = ZOMBIE;
  LOGI("proc exit pid:%d\n", current_proc->pid);
  __asm__ volatile(
    "mov %0, %%cr3\n\t"
    "mov %1, %%esp\n\t"
	"jmp %2"
    : : "r"(entry_pg_dir),
        "r"(PDE_SIZE),
        "r"(sched_loop)
  );
}
