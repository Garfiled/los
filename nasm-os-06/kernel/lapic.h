#pragma once

extern volatile unsigned int* lapic;

int lapicid(void);
void lapicstartap(unsigned char apicid, unsigned int addr);
