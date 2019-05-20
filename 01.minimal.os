	.code16
	.global _start, begtext, begdata, begbss, endtext, enddata, endbss
	.text
	begtext:
	.data
	begdata:
	.bss
	begbss:
	.text
	
	mov	%dl,0x7c4c
	mov	%dl,%al
	call	print_hex
	call	print_nl
	
	mov	$0x08,%ah
	int	$0x13
	jc	bad_read_disk_param
	mov	$0x0,%ah
	mov	%dl,%al
	call	print_hex
	call	print_nl
	jmp 	loop

bad_read_disk_param:
	mov	$0xe,%ah
	mov	$'E',%al
	int	$0x10
	jmp 	loop

loop:
	jmp	loop

print_hex:
        mov $4,%cx
        mov %ax,%dx

print_digit:
        rol $4,%dx      #循环以使低4位用上，高4位移至低4位
        mov $0xe0f,%ax #ah ＝ 请求的功能值，al = 半个字节的掩码
        and %dl,%al
        add $0x30,%al
        cmp $0x3a,%al
        jl outp
        add $0x07,%al

outp:
        int $0x10
        loop print_digit
        ret

print_nl:
        mov $0xe0d,%ax
        int $0x10
        mov $0xa,%al
        int $0x10
        ret
HD:
	.byte 0x0

	.org 510
boot_flag:
	.word 0xAA55
	
	.text
	endtext:
	.data
	enddata:
	.bss
	endbss:
