#pragma once

extern volatile unsigned int* lapic;

void lapicstartap(unsigned char apicid, unsigned int addr);
int lapicid(void);
