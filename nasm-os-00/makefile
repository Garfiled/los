GDB = gdb
LD = ld -m elf_i386 -s

# First rule is run by default
os-image.bin: boot/bootsect.bin
	cat $^ > os-image.bin

run: os-image.bin
	qemu-system-i386 -fda os-image.bin

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.o os-image.bin
	rm -rf boot/*.bin boot/*.o
