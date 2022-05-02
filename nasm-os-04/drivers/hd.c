#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../libc/function.h"
#include "../libc/string.h"
#include "../drivers/screen.h"
#include "../drivers/hd.h"


struct hd_request 
{
	uint32_t lba;
	uint8_t  cmd;
	uint16_t nsects;
	void *buf;
};

static struct hd_request curr_hd_request;

static void hd_interrupt(registers_t *regs) {
	kprint("hd_interrupt:\n");
	kprint("check_status>>>");
	kprint("\n");
	uint8_t status = port_byte_in(HD_PORT_STATUS);
	kprint("status:");
	char status_str[10];
	int_to_ascii(status,status_str);
	kprint(status_str);
	kprint("\n");

	kprint("port_read/write>>>");
	kprint("\n");
	//读取数据
	if (curr_hd_request.cmd == HD_READ)
	{
		port_read(HD_PORT_DATA,curr_hd_request.buf, curr_hd_request.nsects * 256);
	}
	//写入数据
	else if (curr_hd_request.cmd == HD_WRITE)
	{
		port_write(HD_PORT_DATA, curr_hd_request.buf, curr_hd_request.nsects * 256);
	}
	
	kprint("content:");
	kprint_k((char*)curr_hd_request.buf,10);
	kprint("\n");

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
	kprint("hd_rw>>>");
	kprint("\n");
	curr_hd_request.lba = lba;
	curr_hd_request.cmd = cmd;
	curr_hd_request.nsects = nsects;
	curr_hd_request.buf = buf;
	
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
}

void check_hd_status()
{
	uint8_t status = port_byte_in(HD_PORT_STATUS);
	kprint("status:");
	char status_str[10];
	int_to_ascii(status,status_str);
	kprint(status_str);
	kprint("\n");
}

void reset_hd_controller()
{
	kprint("reset:");
	kprint("\n");
	port_byte_out(HD_CMD,4);
	for (int i=1000;i>0;i--);
	port_byte_out(HD_CMD,8);
	for (int i=1000;i>0;i--);
}
