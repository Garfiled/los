#pragma once

#include <stdint.h>

extern uint32_t tick;
extern uint32_t entry_pg_dir[];
extern uint32_t entry_pg_dir2[];

void kernel_main();
void *hd_setup(void *addr);
void user_input(char *input);
void startothers();
void mpenter();
void init_entry_page();
void init_entry_page2();
void scheduler();
void sched_loop();

