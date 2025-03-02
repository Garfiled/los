; Identical to lesson 13's boot sector, but the %included files have new paths
[org 0x7c00]
KERNEL_OFFSET equ 0x1000 ; The same one we used when linking the kernel
SETUP equ 0x7e00
    xor ax,ax
    mov es,ax
    mov ds,ax
    mov ss,ax

    mov [BOOT_DRIVE], dl ; Remember that the BIOS sets us the boot drive in 'dl' on boot
    mov bp, 0x9000
    mov sp, bp

    mov bx, MSG_REAL_MODE 
    call print
    call print_nl
	
    mov dl,[BOOT_DRIVE]
    mov dh,0x3
    mov cl,0x2
    mov bx,SETUP
    call disk_load

    jmp SETUP

%include "boot/print.asm"
%include "boot/print_hex.asm"
%include "boot/disk.asm"
%include "boot/gdt.asm"
%include "boot/32bit_print.asm"
%include "boot/switch_pm.asm"

BOOT_DRIVE db 0 ; It is a good idea to store it in memory because 'dl' may get overwritten
MSG_REAL_MODE db "Started in 16-bit Real Mode", 0

; padding
times 510 - ($-$$) db 0
dw 0xaa55

[bits 16]
    mov ah,0x41
    mov dl,[BOOT_DRIVE]
    mov bx,0x55AA
    int 0x13
    jc req_disk_err
    mov dx,bx
    call print_hex
    call print_nl
    
    mov	ah,0x48
    mov dl,[BOOT_DRIVE]
    mov si,0x9000
    mov word [si],0x1E
    mov word [si+0x2],0x0
    int 0x13
    jc req_disk_err
	
    call load_kernel ; read the kernel from disk
    call switch_to_pm ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
    jmp $ ; Never executed

req_disk_err:
    mov dh,ah
    call print_hex
    call print_nl
    jmp $

[bits 16]
load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print
    call print_nl

    mov bx, KERNEL_OFFSET ; Read from disk and store in 0x1000
    mov dh, 25 ; Our future kernel will be larger, make this big
    mov dl, [BOOT_DRIVE]
    mov cl,0x5
    call disk_load
    ret

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    call KERNEL_OFFSET ; Give control to the kernel
    jmp $ ; Stay here when the kernel returns control to us (if ever)


MSG_PROT_MODE db "Landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0

; padding
times 2048 - ($-$$) db 0
