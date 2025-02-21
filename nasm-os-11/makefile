C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c mm/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h mm/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o kernel/swtch.o} 

# Change this if your cross-compiler is somewhere else
SRC_DIR=./
CC = gcc -fno-pic
GDB = gdb
LD = ld -m elf_i386
# -g: Use debugging symbols in gcc
CFLAGS = -g -ffreestanding -Wall -Wextra -fno-exceptions -m32 -I./
IFLAGS=-I${SRC_DIR}

os-image.bin: boot/bootsect.bin boot/entryother.bin kernel.bin
	cat $^ > os-image.bin
	dd if=/dev/zero of=./os-image.bin.tmp bs=64k count=1
	cat os-image.bin.tmp >> os-image.bin
	head -c65536 os-image.bin > os-image.bin.tmp
	mv os-image.bin.tmp os-image.bin

hello.bin: usr/hello.o
	${LD} -o hello.bin $^

test: hello.bin
	# 文件数量
	@/bin/echo -n -e \\x01\\x00\\x00\\x00 > test
	# 2. 文件元数据条目（共40字节）
	# 文件名（32字节，"hello" + 27个零填充）
	@/bin/echo -n -e "hello" | dd bs=32 conv=sync,notrunc >> test 2>/dev/null
	@/bin/echo -n -e \\xe4\\x21\\x00\\x00 >> test
	@/bin/echo -n -e \\xa4\\x0f\\x00\\x00 >> test
	dd if=/dev/zero bs=3960 count=1 >> test
	cat hello.bin >> test
	# 4. 填充至100KB
	truncate -s 102400 test

# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
kernel.bin: boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x10000 $^ --oformat binary

# Used for debugging purposes
kernel.elf: boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x10000 $^

run: os-image.bin
	qemu-system-i386 -hda os-image.bin -smp 2

# Open the connection to qemu and load our kernel-object file with symbols
debug: os-image.bin kernel.elf
	qemu-system-i386 -s -fda os-image.bin -d guest_errors,int &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# Generic rules for wildcards
# To make an object, always compile from its .c
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} $(IFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image.bin *.elf test
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o libc/*.o mm/*.o fs/*.o usr/*.o
