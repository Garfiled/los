#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../libc/function.h"
#include "../drivers/screen.h"
#include "../drivers/hd.h"

static void hd_interrupt(registers_t *regs) {
	kprint("hd_interrupt:\n");
    UNUSED(regs);
}

void init_hd(uint32_t freq) {
    UNUSED(freq);
    register_interrupt_handler(IRQ14, hd_interrupt);

    /* Send the command */
   port_byte_out(port_byte_in(0x21)&0xfb, 0x21); /* Command port */
   port_byte_out(port_byte_in(0xA1)&0xbf, 0xA1);
}

// CHS request address
#define HD_DATA		0x1f0	/* _CTL when writing */
#define HD_ERROR	0x1f1	/* see err-bits */
#define HD_NSECTOR	0x1f2	/* nr of sectors to read/write */
#define HD_SECTOR	0x1f3	/* starting sector */
#define HD_LCYL		0x1f4	/* starting cylinder */
#define HD_HCYL		0x1f5	/* high byte of starting cyl */
#define HD_CURRENT	0x1f6	/* 101dhhhh , d=drive, hhhh=head */
#define HD_STATUS	0x1f7	/* see status-bits */
#define HD_CMD		0x3f6
/*
static int controller_ready(void)
{
	int retries=100000;

	while (--retries && (port_byte_in(HD_STATUS)&0x80));
	return (retries);
}

static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
{

    register int port asm("dx");
	if (!controller_ready()) {
		kprint("HD controller not ready!");
		return;
    }
	do_hd = intr_addr;
	port_byte_out(hd_info[drive].ctl,HD_CMD);
	port=HD_DATA;
	port_byte_out(hd_info[drive].wpcom>>2,++port);
	port_byte_out(nsect,++port);
	port_byte_out(sect,++port);
	port_byte_out(cyl,++port);
	port_byte_out(cyl>>8,++port);
	port_byte_out(0xA0|(drive<<4)|head,++port);
	outb(cmd,++port);
}
*/

void hd_rw(uint32_t lba,uint8_t  cmd, uint16_t nsects,void *buf)
{
	uint8_t status = 0;
	uint16_t try_times = 0x10;
	do
	{
		//数据缓冲区
		void *buf2 = buf;

		//计算扇区号
		uint8_t lba0 = (uint8_t) (lba & 0xff);
		uint8_t lba1 = (uint8_t) (lba >> 8 & 0xff);
		uint8_t lba2 = (uint8_t) (lba >> 16 & 0xff);
		uint8_t lba3 = (uint8_t) (lba >> 24 & 0xf);

		//IDE0主设备
		lba3 |= 0xe0; // 1110 0000

		//发送读写命令
		port_byte_out(HD_PORT_SECT_COUNT,nsects);
		port_byte_out(HD_PORT_LBA0, lba0);
		port_byte_out(HD_PORT_LBA1, lba1);
		port_byte_out(HD_PORT_LBA2, lba2);
		port_byte_out(HD_PORT_LBA3, lba3);
		port_byte_out(HD_PORT_COMMAND, cmd);

		int32_t check_status_retry = 1000;
		while (check_status_retry >0 && ((port_byte_in(HD_PORT_STATUS) & 0xc0) != 0x40))
		{
			check_status_retry--;
		}
		if (check_status_retry<=0)
			continue;
		//读取数据
		if (cmd == HD_READ)
		{
			port_read(HD_PORT_DATA,buf2, nsects * 256);
		}
		//写入数据
		else if (cmd == HD_WRITE)
		{
			port_write(HD_PORT_DATA, buf2, nsects * 256);
		}
		//取得操作后的设备状态
		status = port_byte_in(HD_PORT_STATUS);
		if (((cmd == HD_READ) && (status != 0x50)) || ((cmd == HD_WRITE) && (status != 0x90)))
		{
		}
		//尝试try_times次后跳出
		if (try_times-- == 0)
		{
			break;
		}
	}
	//如果操作成功，退出
	while (((cmd == HD_READ) && (status != 0x50)) || ((cmd == HD_WRITE) && (status != 0x90)));
}
