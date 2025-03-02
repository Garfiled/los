;Context switch

STACK_SAVE_ADDR equ 0xC0000000 + 4096 - 4
  global swtch
swtch:
  mov eax, [esp + 4] ; stack
  mov edx, [esp + 8] ; entry

  push ebp
  push ebx
  push esi
  push edi

  ; save
  mov [STACK_SAVE_ADDR], esp

  mov esp, eax

  pop edi
  pop esi
  pop ebx
  pop ebp

  call edx
  mov esp, [STACK_SAVE_ADDR]

  pop edi
  pop esi
  pop ebx
  pop ebp
  ret
