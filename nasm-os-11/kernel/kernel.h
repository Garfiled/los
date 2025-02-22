#pragma once

#include <stdint.h>

extern uint32_t tick;
extern uint32_t entry_pg_dir[];
extern uint32_t entry_pg_dir2[];

#ifdef __cplusplus
extern "C" {
#endif
void kernel_main();
#ifdef __cplusplus
}
#endif

void* hd_setup(char *addr);
void user_input(char *input);
void startothers();
void mpenter();
void init_entry_page();
void init_entry_page2();
void scheduler();

