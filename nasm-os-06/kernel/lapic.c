#include "lapic.h"
#include "../cpu/ports.h"


#define ID      (0x0020/4)   // ID
#define CMOS_PORT    0x70
#define CMOS_RETURN  0x71

#define ICRLO   (0x0300/4)   // Interrupt Command
#define INIT       0x00000500   // INIT/RESET
#define STARTUP    0x00000600   // Startup IPI
#define ASSERT     0x00004000   // Assert interrupt (vs deassert)
#define LEVEL      0x00008000   // Level triggered
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]

volatile unsigned int *lapic;

int lapicid(void)
{
  if (!lapic)
    return 0;
  return lapic[ID] >> 24;
}

static void lapicw(int index, int value)
{
  lapic[index] = value;
  lapic[ID];  // wait for write to finish, by reading
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
}

void lapicstartap(unsigned char apicid, unsigned int addr)
{
  int i;
  unsigned short *wrv;

  // "The BSP must initialize CMOS shutdown code to 0AH
  // and the warm reset vector (DWORD based at 40:67) to point at
  // the AP startup code prior to the [universal startup algorithm]."
  port_byte_out(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
  port_byte_out(CMOS_PORT+1, 0x0A);
  wrv = (unsigned short*)(0x40<<4 | 0x67);  // Warm reset vector
  wrv[0] = 0;
  wrv[1] = addr >> 4;

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(ICRHI, apicid<<24);
  lapicw(ICRLO, INIT | LEVEL | ASSERT);
  microdelay(200);
  lapicw(ICRLO, INIT | LEVEL);
  microdelay(100);    // should be 10ms, but too slow in Bochs!

  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for(i = 0; i < 2; i++){
    lapicw(ICRHI, apicid<<24);
    lapicw(ICRLO, STARTUP | (addr>>12));
    microdelay(200);
  }
}
