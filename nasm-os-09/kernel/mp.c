// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "kernel/mp.h"
#include "cpu/x86.h"
#include "kernel/proc.h"
#include "libc/kprint.h"
#include "libc/string.h"
#include "cpu/ports.h"
#include "kernel/lapic.h"

struct cpu cpus[NCPU];
int ncpu;
unsigned char ioapicid;

static unsigned char sum(unsigned char *addr, int len)
{
  int i, sum;
  sum = 0;
  for(i=0; i<len; i++)
    sum += addr[i];
  return sum;
}

// Look for an MP structure in the len bytes at addr.
static struct mp* mpsearch1(unsigned int a, int len)
{
  unsigned char *e, *p, *addr;

  addr = (unsigned char*)(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp)) {
    if(STRCMPN(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0) {
      kprintf("Found MP floating pointer at %x\n", p);
      return (struct mp*)p;
	}
  }
  kprintf("No MP floating pointer found in range %x-%x\n", addr, e);
  return 0;
}

// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp* mpsearch(void)
{
  unsigned char *bda;
  unsigned int p;
  struct mp *mp;

  bda = (unsigned char *) P2V(0x400);
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
    if((mp = mpsearch1(p, 1024))) {
	  kprintf("Found MP floating pointer debug1 at %x\n", mp);
      return mp;
	}
  } else {
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024))) {
	  kprintf("Found MP floating pointer debug2 at %x\n", mp);
      return mp;
	}
  }
  kprintf("Checking BIOS ROM at F0000\n");
  return mpsearch1(0xF0000, 0x10000);
}

// Search for an MP configuration table.  For now,
// don't accept the default configurations (physaddr == 0).
// Check for correct signature, calculate the checksum and,
// if correct, check the version.
// To do: check extended table checksum.
static struct mpconf* mpconfig(struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if((mp = mpsearch()) == 0 || mp->physaddr == 0) {
    kprintf("MP floating pointer not found\n");
    return 0;
  }
  conf = (struct mpconf*) P2V((unsigned int) mp->physaddr);
  //kprintf("Found valid MP configuration table at %x\n", conf);
  //kprintf("mpconf: %x %x %x %x\n", mp, mp->physaddr, conf, conf->signature);
  //for (int i = 0; i < 16; i++) {
  //  kprintf("%d ", ((char*)conf->signature)[i]);
  //}
  //kprintf("\n");
  if(STRCMPN(conf, "PCMP", 4) != 0) {
    kprintf("Invalid MP configuration table signature: %d %d %d %d\n", conf->signature[0],
      conf->signature[1], conf->signature[2], conf->signature[3]);
    return 0;
  }
  if(sum((unsigned char*)conf, conf->length) != 0) {
    kprintf("Invalid MP configuration table checksum: %x %d %x\n", conf, conf->length, mp->physaddr);
    return 0;
  }
  *pmp = mp;
  return conf;
}

void mpinit(void)
{
  unsigned char *p, *e;
  int ismp;
  struct mp *mp;
  struct mpconf *conf;
  struct mpproc *proc;
  struct mpioapic *ioapic;

  if((conf = mpconfig(&mp)) == 0) {
    kprintf("Expect to run on an SMP!\n");
    return;
  }
  ismp = 1;
  lapic = (unsigned int*)conf->lapicaddr;
  kprintf("mpinit: %d %x\n", *lapic, lapic);
  for(p=(unsigned char*)(conf+1), e=(unsigned char*)conf+conf->length; p<e; ){
    switch(*p){
    case MPPROC:
      proc = (struct mpproc*)p;
      if(ncpu < NCPU) {
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
        kprintf("cpu %d %d\n", ncpu, proc->apicid);
        ncpu++;
      }
      p += sizeof(struct mpproc);
      continue;
    case MPIOAPIC:
      ioapic = (struct mpioapic*)p;
      ioapicid = ioapic->apicno;
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      p += 8;
      continue;
    default:
      ismp = 0;
      break;
    }
  }
  if(!ismp) {
    kprintf("Didn't find a suitable machine!\n");
    return;
  }
  kprintf("mpinit cpu num = %d\n", ncpu);

  if(mp->imcr_present){
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    port_byte_out(0x22, 0x70);
    port_byte_out(0x23, port_byte_in(0x23) | 1);
  }
}

void mpinit1(void)
{
  struct mpconf* conf;
  struct mp* mp;

  if ((conf = mpconfig(&mp)) == 0) {
    kprintf("Expect to run on an SMP!\n");
	hang();
    return;
  }
  uint32_t total_length = conf->length;
  kprintf("MP configuration table length: %d %d %x\n", conf->length, total_length, conf);
  kprintf("sizeof mpconf:%d mpproc:%d\n", sizeof(struct mpconf), sizeof(struct mpproc));
  if (total_length <= sizeof(struct mpconf) || total_length > 4096) {
    kprintf("Invalid MP configuration table length\n");
    return;
  }

  kprintf("MP configuration table contents: %x %d\n",conf, conf->entry);
  int print_length = total_length < 256 ? total_length : 256;
  for (int i = 0; i < print_length; i++) {
    kprintf("%d ", ((unsigned char*)conf)[i]);
    if ((i + 1) % 16 == 0) kprintf("\n");
	if (i == 44-1) kprintf("\n\n");
  }
  kprintf("\n");
  // 解析 MP 配置表

  unsigned char *p, *e;
  struct mpproc *proc;
  int ismp = 1;
  for(p=(unsigned char*)(conf+1), e=(unsigned char*)conf+conf->length; p<e; ){
	kprintf("debug>:%x %d %d\n", p, p[0], p[1]);
    switch(*p){
    case MPPROC:
      proc = (struct mpproc*)p;
      if(ncpu < NCPU) {
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
        ncpu++;
      }
      p += 20;
      continue;
    case MPIOAPIC:
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      p += 8;
      continue;
    default:
      ismp = 0;
      break;
    }
  }
  UNUSED(ismp);
  kprintf("Found %d CPUs\n", ncpu);
}
