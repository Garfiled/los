all: minimal
minimal: minimal.os
	as --32 -o minimal.o minimal.os
	ld -m elf_i386 -Ttext 0 -o minimal.bin minimal.o
	objcopy --dump-section .text=minimal minimal.bin


clean:
	rm -f kernel minimal *.o *.bin
usb:
	sudo dd if=./minimal of=/dev/sdb bs=512 count=300
start:
	qemu-system-i386 -fda los.img
