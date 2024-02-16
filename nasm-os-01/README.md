## os minimal kernel use nasm 
```sh
make
qemu-system-i386 -hda os.image.bin
```
## From ChatGPT

这段x86汇编代码是一个简单的引导加载程序，它的作用是加载内核并将控制权转移到内核。我会解释其中关键部分的功能：

org 0x7c00：设置代码的起始地址为0x7c00，这是x86架构的标准引导加载程序加载位置。

KERNEL_OFFSET equ 0x1000：定义了内核加载到内存的偏移量。这个偏移量通常是在编译内核时确定的。

xor ax, ax、mov es, ax、mov ds, ax、mov ss, ax：将段寄存器设置为0，确保在实模式下使用正确的段地址。

mov [BOOT_DRIVE], dl：将BIOS提供的引导驱动器号存储在BOOT_DRIVE变量中。

mov bp, 0xff00、mov sp, bp：设置栈指针的初始位置。

call print、call print_nl：调用打印函数，显示一些启动信息。

call load_kernel：调用加载内核的函数。

call switch_to_pm：调用切换到保护模式的函数。

jmp $：这一行永远不会执行，它仅是一个占位符，用于确保程序不会继续执行未知的指令。

这段代码通过汇编指令来控制硬件、加载内核、切换到保护模式等，最终实现将控制权转移到内核的目的。内核偏移量和加载位置等常量都被硬编码在程序中，这样在编译时就确定了它们的值。
