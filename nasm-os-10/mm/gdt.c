#include "mm/gdt.h"
#include "libc/string.h"
#include "libc/kprint.h"

static gdt_ptr_t gdt_ptr;
static gdt_entry_t gdt_entries[8];

static tss_entry_t tss_entry;

static void refresh_gdt()
{
  load_gdt(&gdt_ptr);
}

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
  gdt_entries[num].limit_low = (limit & 0xFFFF);
  gdt_entries[num].base_low = (base & 0xFFFF);
  gdt_entries[num].base_middle = (base >> 16) & 0xFF;
  gdt_entries[num].access = access;
  gdt_entries[num].attributes = (limit >> 16) & 0x0F;
  gdt_entries[num].attributes |= ((flags << 4) & 0xF0);
  gdt_entries[num].base_high = (base >> 24) & 0xFF;
}

static void write_tss(uint32_t num, uint16_t ss0, uint32_t esp0)
{
  MEMSET(&tss_entry, 0, sizeof(tss_entry_t));
  tss_entry.ss0 = ss0;
  tss_entry.esp0 = esp0;
  tss_entry.iomap_base = sizeof(tss_entry_t);

  // Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
  // segments should be loaded when the processor switches to kernel mode. Therefore
  // they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
  // but with the last two bits set, making 0x0b and 0x13. The setting of these bits
  // sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
  // to switch to kernel mode from ring 3.
  tss_entry.cs = SELECTOR_K_CODE | RPL3;
  tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = SELECTOR_K_DATA | RPL3;

  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = base + sizeof(tss_entry);
  gdt_set_gate(num, base, limit, DESC_P | DESC_DPL_0 | DESC_S_SYS | DESC_TYPE_TSS, 0x0);
}

void init_gdt() {
  gdt_ptr.limit = (sizeof(gdt_entry_t) * 8) - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  // reserved
  gdt_set_gate(0, 0, 0, 0, 0);

  // kernel code
  gdt_set_gate(1, 0, 0xFFFFF, DESC_P | DESC_DPL_0 | DESC_S_CODE | DESC_TYPE_CODE, FLAG_G_4K | FLAG_D_32);
  // kernel data
  gdt_set_gate(2, 0, 0xFFFFF, DESC_P | DESC_DPL_0 | DESC_S_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);
  // video: only 8 pages
  gdt_set_gate(3, 0, 7, DESC_P | DESC_DPL_0 | DESC_S_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);

  // user code
  gdt_set_gate(4, 0, 0xBFFFF, DESC_P | DESC_DPL_3 | DESC_S_CODE | DESC_TYPE_CODE, FLAG_G_4K | FLAG_D_32);
  // user data
  gdt_set_gate(5, 0, 0xBFFFF, DESC_P | DESC_DPL_3 | DESC_S_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);

  // tss:
  write_tss(6, 0x10, 0x0);

   // 新增 GS 专用段 (0x38)
  // 基地址 = 3GB + 8KB = 0xC0000000 + 0x2000 = 0xC0002000
  // 段限制 = 0x1FFF (若需要 8KB 空间)
  // 这里保持与内核数据段相同的 4GB 范围，实际通过分页隔离
  gdt_set_gate(7,
    0xC0002000,          // 基地址
    0x1FFF,             // 段限制
    DESC_P | DESC_DPL_0 | DESC_S_DATA | DESC_TYPE_DATA,
    FLAG_G_4K | FLAG_D_32
  );

  refresh_gdt();
  refresh_tss();

  uint32_t gs_addr = 0;
  asm volatile ("movl %%gs, %0" : "=r"(gs_addr));
  uint32_t gs_value = 0;
  asm volatile ("movl %%gs:0, %0" : "=r"(gs_value));
  LOGD("gs_addr:%x gs_value:%x\n", gs_addr, gs_value);
}

void update_tss_esp(uint32_t esp) {
  tss_entry.esp0 = esp;
}

// 加载GDT
inline void load_gdt(struct gdt_ptr *ptr)
{
    asm volatile (
        "lgdt %0\n\t"
        // 更新数据段选择子
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%ss\n\t"
        // 更新GS段选择子
        "mov $0x38, %%ax\n\t"
        "mov %%ax, %%gs\n\t"
        // 远跳转刷新CS
        "ljmp $0x08, $1f\n\t"
        "1:"
        : : "m" (*ptr) : "eax", "memory"
    );
}

// 刷新TSS选择子
inline void refresh_tss(void)
{
    asm volatile (
        "ltr %0"
        : : "r" ((uint16_t)0x30)
        : "eax", "memory"
    );
}
