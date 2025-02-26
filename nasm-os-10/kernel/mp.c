// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "kernel/mp.h"
#include "kernel/proc.h"
#include "kernel/lapic.h"
#include "libc/kprint.h"
#include "libc/string.h"
#include "cpu/ports.h"

struct cpu cpus[NCPU];
int ncpu;
unsigned char ioapicid;

static unsigned char
sum(unsigned char *addr, int len)
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

  addr = P2V(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp))
    if(STRCMPN(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
      return (struct mp*)p;
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
    if((mp = mpsearch1(p, 1024)))
      return mp;
  } else {
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024)))
      return mp;
  }
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

  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) P2V((unsigned int) mp->physaddr);
  if(STRCMPN(conf, "PCMP", 4) != 0)
    return 0;
  if(conf->version != 1 && conf->version != 4)
    return 0;
  if(sum((unsigned char*)conf, conf->length) != 0)
    return 0;
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

  if(mp->imcrp){
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    port_byte_out(0x22, 0x70);
    port_byte_out(0x23, port_byte_in(0x23) | 1);
  }
}
