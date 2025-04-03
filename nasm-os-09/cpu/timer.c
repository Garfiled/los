#include "cpu/timer.h"
#include "cpu/isr.h"
#include "libc/kprint.h"
#include "kernel/proc.h"
#include "cpu/ports.h"

uint32_t tick = 0;

static void timer_callback(registers_t *regs)
{
  __atomic_fetch_add(&tick, 1, __ATOMIC_SEQ_CST);
  if (tick % 1000 == 0) {
    LOGI("timer_callback tick tick:%d current_proc:%x eip:%x esp:%x\n", tick, current_proc, eip(), esp());
  }
  if (tick % 10== 0 && current_proc != NULL && current_proc->state != ZOMBIE && current_proc->state != UNUSED) {
	current_proc->priority -= 10;
	//puts("time cb has proc\n");
    // 保存被中断进程的上下文
    if(current_proc->priority <= 0) {
	  uint32_t esp_before_int = regs->esp + 20;
	  //puts("time cb need schedule proc\n");
      current_proc->context.ebx = regs->ebx;
      current_proc->context.edi = regs->edi;
      current_proc->context.esi = regs->esi;
      current_proc->context.ebp = regs->ebp;
      current_proc->context.esp = esp_before_int; // eflags + cs + eip + status + i_no
      current_proc->context.eip = regs->eip;
      current_proc->context.eflags = regs->eflags;
       // 更新时间片
      current_proc->state = RUNNABLE;
      LOGD("time_cb need schedule current_proc:%x pid:%d priority:%d reg_eip:%x esp_before_int:%x tick:%d\n",current_proc, current_proc->pid,
			  current_proc->priority, regs->eip, esp_before_int, tick);
      schedule(); // 触发调度
    }
  }
}

void init_timer(uint32_t freq)
{
  /* Get the PIT value: hardware clock at 1193180 Hz */
  uint32_t divisor = 1193180 / freq;
  uint8_t low  = (uint8_t)(divisor & 0xFF);
  uint8_t high = (uint8_t)( (divisor >> 8) & 0xFF);
  /* Send the command */
  port_byte_out(0x43, 0x36); /* Command port */
  port_byte_out(0x40, low);
  port_byte_out(0x40, high);

  /* Install the function we just wrote */
  register_interrupt_handler(IRQ0, timer_callback);
}

