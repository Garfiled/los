// See MultiProcessor Specification Version 1.[14]
#pragma once

#include <stdint.h>
#define NCPU 8
#define P2V(a) ((void *)(((char *) (a))))

void mpinit(void);

struct mp {
    char signature[4];        // "_MP_"
    uint32_t physaddr;        // MP 配置表的物理地址（32位）
    uint8_t length;           // 浮动指针结构长度（通常为 1）
    uint8_t spec_rev;         // 规范版本（如 0x14 对应 1.4）
    uint8_t checksum;         // 所有字节和为 0
    uint8_t mp_type;          // MP 系统配置类型
    uint8_t imcr_present;     // IMCR 支持标志
    uint8_t reserved[3];      // 保留
} __attribute__((packed));    // 总大小应为 16 字节

struct mpconf {
    char signature[4];            // "PCMP"
    uint16_t length;              // 表长度（以 16 字节为单位）
    uint8_t spec_rev;             // 规范版本
    uint8_t checksum;             // 所有字节和为 0
    char oem_id[8];               // OEM 标识符
    char product_id[12];          // 产品标识符
    uint32_t oem_table_ptr;       // OEM 表指针（物理地址）
    uint16_t oem_table_length;    // OEM 表长度
    uint16_t entry_count;         // 条目数量
    uint32_t lapic_addr;          // Local APIC 地址（物理地址）
    uint16_t ext_table_length;    // 扩展表长度
    uint8_t ext_table_checksum;   // 扩展表校验和
    uint8_t reserved;             // 保留
} __attribute__((packed));        // 总大小应为 44 字节

struct mpproc {         // processor table entry
  unsigned char type;                   // entry type (0)
  unsigned char apicid;                 // local APIC id
  unsigned char version;                // local APIC verison
  unsigned char flags;                  // CPU flags
    #define MPBOOT 0x02           // This proc is the bootstrap processor.
  unsigned char signature[4];           // CPU signature
  unsigned int feature;                 // feature flags from CPUID instruction
  unsigned char reserved[8];
};

struct mpioapic {       // I/O APIC table entry
  unsigned char type;                   // entry type (2)
  unsigned char apicno;                 // I/O APIC id
  unsigned char version;                // I/O APIC version
  unsigned char flags;                  // I/O APIC flags
  unsigned int *addr;                  // I/O APIC address
};

// Table entry types
#define MPPROC    0x00  // One per processor
#define MPBUS     0x01  // One per bus
#define MPIOAPIC  0x02  // One per I/O APIC
#define MPIOINTR  0x03  // One per bus interrupt source
#define MPLINTR   0x04  // One per system interrupt source

//PAGEBREAK!
// Blank page.
