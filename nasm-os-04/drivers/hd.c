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

	int status = hd_wait(1);
	if (status!=0)
	{
		kprint("hd_status:");
		kprintInt(status);
		kprint("\n");
		return;
	}
	kprint("port_read>>>");
	kprint("\n");
	//读取数据
	if (curr_hd_request.cmd == HD_READ)
	{
		port_read(HD_PORT_DATA,curr_hd_request.buf, curr_hd_request.nsects * 256);
	}
	
	kprint("content:");
	kprint_k((char*)curr_hd_request.buf,10);
	kprint("\n");

    UNUSED(regs);
}

static int
hd_wait(int checkerr)
{
  int r;
  uint32_t try = 1000000;
  while(((r = port_byte_in(HD_PORT_STATUS)) & (HD_BSY|HD_DRDY)) != HD_DRDY && try>0)
  	try--;
  if(checkerr && (r & (HD_DF|HD_ERR)) != 0)
    return r;
  if (try<=0)
  	return -1;
  return 0;
}

void init_hd(uint32_t freq) {
    register_interrupt_handler(IRQ14, hd_interrupt);

    port_byte_out(0x1f6, 0xe0 | (0<<4));

    /* Send the command */
   // port_byte_out(port_byte_in(0x21)&0xfb, 0x21);  Command port 
   // port_byte_out(port_byte_in(0xA1)&0xbf, 0xA1);
}

void hd_rw(uint32_t lba,uint8_t  cmd, uint16_t nsects,void *buf)
{
	kprint("hd_rw>>>");
	kprint("\n");
	int status = hd_wait(0);
	if (status!=0)
	{
		kprint("hd_status:");
		kprintInt(status);
		kprint("\n");
		return;
	}

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
	port_byte_out(0x3f6,0);
	port_byte_out(HD_PORT_SECT_COUNT,nsects);
	port_byte_out(HD_PORT_LBA0, lba0);
	port_byte_out(HD_PORT_LBA1, lba1);
	port_byte_out(HD_PORT_LBA2, lba2);
	port_byte_out(HD_PORT_LBA3, lba3);
	port_byte_out(HD_PORT_COMMAND, cmd);

	if (cmd == HD_WRITE)
	{
		port_write(HD_PORT_DATA, buf, nsects * 256);
	}
}

void check_hd_status()
{
	uint8_t status = port_byte_in(HD_PORT_STATUS);
	kprint("status:");
	kprintInt(status);
	kprint("\n");
}