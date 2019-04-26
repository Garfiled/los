#all: los.img
all: minimal
minimal: minimal.os
	as --32 -o minimal.o minimal.os
	ld -m elf_i386 -Ttext 0 -o minimal.bin minimal.o
	objcopy --dump-section .text=minimal minimal.bin

los.img: bootsect setup kernel
	cat bootsect > los.img
	dd if=/dev/zero of=setup.tmp bs=512 count=4
	dd if=setup of=setup.tmp bs=2048 count=1 conv=notrunc
	cat setup.tmp >> los.img
	cat kernel >> los.img

bootsect: bootsect.s
	as --32 -o bootsect.o bootsect.s
	ld -m elf_i386 -Ttext 0 -o bootsect.bin bootsect.o
	objcopy --dump-section .text=bootsect bootsect.bin

setup: setup.s
	as --32 -o setup.o setup.s
	ld -m elf_i386 -Ttext 0 -o setup.bin setup.o
	objcopy --dump-section .text=setup setup.bin


kernel: head.o main.o
	ld -m elf_i386 -Ttext 0x0 -o kernel.bin head.o main.o
	objcopy --dump-section .text=kernel kernel.bin

main.o: main.c
	gcc -m32 -c main.c -o main.o

head.o: head.s
	as --32 -o head.o head.s

clean:
	rm -f bootsect setup kernel minimal *.o *.bin *.tmp *.img
usb:
	sudo dd if=./los.img of=/dev/sdb bs=512 count=300
start:
	qemu-system-i386 -fda los.img
