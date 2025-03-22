;Context switch
  global switch_context
switch_context:
    ; 保存旧上下文
    mov eax, [esp + 4]    ; old
    pushfd
    push ebp
    push ebx
    push esi
    push edi
    mov [eax], esp        ; 保存旧栈指针
    
    ; 加载新上下文
    mov eax, [esp + 28]   ; new
    mov esp, [eax]
    
    ; 恢复新上下文
    pop edi
    pop esi
    pop ebx
    pop ebp
    popfd
    ret
