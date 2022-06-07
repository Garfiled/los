;Context switch

  global swtch
swtch:
  mov eax, [esp + 4] ; stack
  mov edx, [esp + 8] ; entry

  push ebp
  push ebx
  push esi
  push edi

  mov esp, eax
  pop edi
  pop esi
  pop ebx
  pop ebp
  call edx

  pop edi
  pop esi
  pop ebx
  pop ebp
  ret
