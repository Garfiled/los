[bits 16]
start_other:
  cli
  ; Zero data segment registers DS, ES, and SS.
  xor ax, ax
  mov es, ax
  mov ds, ax
  mov ss, ax

  lgdt [gdt_descriptor]
  mov eax, cr0
  or eax, 0x1
  mov cr0, eax
  jmp CODE_SEG:start_other32

%include "boot/gdt.asm"

[bits 32]
start_other32:
  mov ax, DATA_SEG ; 5. update the segment registers
  mov ds, ax
  mov ss, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; Use entrypgdir as our initial page table
  mov eax, [start_other - 12]
  mov cr3, eax

  ; Turn on paging. 
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  mov esp, [start_other - 4]
  mov eax, start_other - 8
  call eax

spin:
  jmp spin

times 2048 - ($-$$) db 0
