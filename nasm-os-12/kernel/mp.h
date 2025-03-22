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

struct mpconf {         // configuration table header
  unsigned char signature[4];           // "PCMP"
  unsigned short length;                // total table length
  unsigned char version;                // [14]
  unsigned char checksum;               // all bytes must add up to 0
  unsigned char product[20];            // product id
  unsigned int *oemtable;               // OEM table pointer
  unsigned short oemlength;             // OEM table length
  unsigned short entry;                 // entry count
  unsigned int *lapicaddr;              // address of local APIC
  unsigned short xlength;               // extended table length
  unsigned char xchecksum;              // extended table checksum
  unsigned char reserved;
} __attribute__((packed));

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
