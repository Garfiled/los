#include "kernel/elf.h"
#include "cpu/x86.h"
#include "libc/string.h"
#include "libc/kprint.h"

int load_elf(char* content, uint32_t* entry_addr)
{
  elf32_ehdr_t* elf_header = (elf32_ehdr_t*)content;

  // Verify magic number
  if (elf_header->e_ident[0] != 0x7f) {
    return -1;
  }
  if (elf_header->e_ident[1] != 'E') {
    return -1;
  }
  if (elf_header->e_ident[2] != 'L') {
    return -1;
  }
  if (elf_header->e_ident[3] != 'F') {
    return -1;
  }
  // kprintf("load_elf:%d\n", elf_header->e_phnum);

  // Load each section.
  elf32_phdr_t* program_header = (elf32_phdr_t*)(content + elf_header->e_phoff);
  for (uint32_t i = 0; i < elf_header->e_phnum; i++) {
    if (program_header->p_type == 0) {
      continue;
    }
	/*
    kprintf("load section to vaddr=%x offset=%d size=%d type=%d\n", program_header->p_vaddr,
                                                                    program_header->p_offset,
                                                                    program_header->p_filesz,
                                                                    program_header->p_type);
    kprint_hex_n((char*)(content + program_header->p_offset), 20);
    kprintf("\n");
	*/
    MEMMOVE((void*)program_header->p_vaddr,
            content + program_header->p_offset,
            program_header->p_filesz);
    program_header = (elf32_phdr_t*)((uint32_t)program_header + elf_header->e_phentsize);
  }

  *entry_addr = elf_header->e_entry;
  return 0;
}
