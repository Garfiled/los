#include "cpu/isr.h"
#include "cpu/ports.h"
#include "libc/function.h"
#include "libc/string.h"
#include "drivers/screen.h"
#include "drivers/hd.h"
#include "mm/alloc.h"
#include "libc/kprint.h"

struct hd_request
{
  uint32_t lba;
  uint8_t  cmd;
  uint16_t nsects;
  void *buf;
};

int open_debug = 0;

static struct hd_request curr_hd_request;

static void hd_interrupt(registers_t *regs) {
  UNUSED(regs);
  //uint8_t status = port_byte_in(HD_PORT_STATUS);
  //kprintf("--hd_interrupt status:%d\n", status);
}

void init_hd()
{
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

void hd_wait_ready()
{
    // 等待设备不再忙（BUSY位为0）
    while (port_byte_in(HD_PORT_STATUS) & 0x80) { // 检查Bit7 (BUSY)
        // 可选：添加短暂延迟或让出CPU以避免忙等待
    }

    // 等待设备就绪（READY位为1且BUSY位为0，即0x40）
    while ((port_byte_in(HD_PORT_STATUS) & 0xC0) != 0x40) { // 检查Bit7和Bit6
        // 同样可考虑延迟
    }
}

void hd_rw(bool is_master_device, uint32_t lba, uint8_t cmd, uint16_t nsects, void *buf)
{
  // kprintf("hd_rw: %x %x %x %x\n", lba, cmd, nsects, buf);
  hd_wait_ready();
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
  if (is_master_device) {
    lba3 |= 0xe0; // 1110 0000
  } else {
    lba3 |= 0xf0; // 1111 0000
  }

  //发送读写命令
  port_byte_out(HD_PORT_SECT_COUNT, nsects);
  port_byte_out(HD_PORT_LBA0, lba0);
  port_byte_out(HD_PORT_LBA1, lba1);
  port_byte_out(HD_PORT_LBA2, lba2);
  port_byte_out(HD_PORT_LBA3, lba3);
  port_byte_out(HD_PORT_COMMAND, cmd);

  hd_wait_ready();

  if (curr_hd_request.cmd == HD_READ) {
    port_read(HD_PORT_DATA, curr_hd_request.buf, curr_hd_request.nsects * 256);
  } else if (curr_hd_request.cmd == HD_WRITE) {
    port_write(HD_PORT_DATA, curr_hd_request.buf, curr_hd_request.nsects * 256);
  }
}

void read_hd(bool is_master_device, char* buf, uint32_t offset, uint32_t size)
{
  uint32_t offset_align = offset / 4096 * 4096;
  uint32_t size_align = 0;
  if ((offset + size - offset_align) % 4096 == 0) {
    size_align = offset + size - offset_align;
  } else {
    size_align = ((offset + size - offset_align) / 4096 + 1) * 4096;
  }
  char *buf_align = (char*)alloc_mm_align(size_align);
  //kprintf("read_hd: buf=%x offset=%x size=%x\n", buf_align, offset_align, size_align);
  hd_rw(is_master_device, offset_align/512, HD_READ, size_align/512, buf_align);
  // kprint("--read_hd_data: ");
  //kprint_hex_n((char*)curr_hd_request.buf, 10);
  //kprint("\n");
  MEMMOVE(buf, buf_align + offset - offset_align, size);
  free_mm(buf_align);
}

void read_hd_split(bool is_master_device, char* buf, uint32_t offset, uint32_t size)
{
  uint32_t offset_align = offset / 4096 * 4096;
  uint32_t size_align = 0;
  if ((offset + size - offset_align) % 4096 == 0) {
    size_align = offset + size - offset_align;
  } else {
    size_align = ((offset + size - offset_align) / 4096 + 1) * 4096;
  }
  char *buf_align = (char*)alloc_mm_align(size_align);
  if (size_align > 512) {
    for (uint32_t i=0;i<size_align/512;i++) {
      hd_rw(is_master_device, (offset_align + 512 * i)/512, HD_READ, 512/512, buf_align + i * 512);
    }
  }
  MEMMOVE(buf, buf_align + offset - offset_align, size);
  free_mm(buf_align);
}

void write_hd_split(bool is_master_device, char* buf, uint32_t offset, uint32_t size)
{
  uint32_t offset_align = offset / 4096 * 4096;
  uint32_t size_align = 0;
  if ((offset + size - offset_align) % 4096 == 0) {
    size_align = offset + size - offset_align;
  } else {
    size_align = ((offset + size - offset_align) / 4096 + 1) * 4096;
  }
  char *buf_align = (char*)alloc_mm_align(size_align);
  read_hd_split(is_master_device, buf_align, offset_align, size_align);
  memcpy(buf_align + offset - offset_align, buf, size);
  if (size_align > 512) {
    for (uint32_t i=0;i<size_align/512;i++) {
      hd_rw(is_master_device, (offset_align + 512 * i)/512, HD_WRITE, 512/512, buf_align + i * 512);
    }
  }
  free_mm(buf_align);
}

void check_hd_status()
{
  uint8_t status = port_byte_in(HD_PORT_STATUS);
  kprint("hd_status: ");
  kprint_int(status);
  kprint("\n");
}

void reset_hd_controller()
{
  //kprint("reset:\n");
  port_byte_out(HD_CMD,4);
  for (int i=1000;i>0;i--);
  port_byte_out(HD_CMD,8);
  for (int i=1000;i>0;i--);
}
