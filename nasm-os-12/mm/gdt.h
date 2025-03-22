#pragma once

#include <stdint.h>

// ****************************************************************************
#define FLAG_G_4K  (1 << 3)
#define FLAG_D_32  (1 << 2)

#define DESC_P     (1 << 7)

#define DESC_DPL_0   (0 << 5)
#define DESC_DPL_1   (1 << 5)
#define DESC_DPL_2   (2 << 5)
#define DESC_DPL_3   (3 << 5)

#define DESC_S_CODE   (1 << 4)
#define DESC_S_DATA   (1 << 4)
#define DESC_S_SYS    (0 << 4)

#define DESC_TYPE_CODE  0x8   // r/x non-conforming code segment
#define DESC_TYPE_DATA  0x2   // r/w data segment
#define DESC_TYPE_TSS   0x9

// ****************************************************************************

#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE   ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA   ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK  SELECTOR_K_DATA
#define SELECTOR_K_GS     ((3 << 3) + (TI_GDT << 2) + RPL0)  // video segment
#define SELECTOR_U_CODE   ((4 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA   ((5 << 3) + (TI_GDT << 2) + RPL3)


// ****************************************************************************
struct gdt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t  base_middle;
  uint8_t  access;
  uint8_t  attributes;
  uint8_t  base_high;
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;

// TSS entry
struct tss_entry_struct {
  uint32_t prev_tss;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;


// ****************************************************************************
void init_gdt();

void update_tss_esp(uint32_t esp);

void load_gdt(gdt_ptr_t*);
void refresh_tss();
