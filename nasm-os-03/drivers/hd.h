#ifndef _HD_H
#define _HD_H

void init_hd();
void hd_rw(uint32_t,uint8_t, uint16_t,void *);

//  LBS request address
#define HD_PORT_DATA            0x1f0
#define HD_PORT_ERROR           0x1f1
#define HD_PORT_SECT_COUNT      0x1f2
#define HD_PORT_LBA0            0x1f3
#define HD_PORT_LBA1            0x1f4
#define HD_PORT_LBA2            0x1f5
#define HD_PORT_LBA3            0x1f6
#define HD_PORT_STATUS          0x1f7
#define HD_PORT_COMMAND         0x1f7
#define HD_READ                 0x20
#define HD_WRITE                0x30

#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr))

#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr))

#endif
