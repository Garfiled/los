#pragma once

#define PAGE_ALIGN_SIZE (4096)
#define PAGE_DIR			(0x600000)
#define PAGE_TABLE		(PAGE_DIR + PAGE_ALIGN_SIZE)

extern void install_page();
