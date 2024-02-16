#pragma once

#include "cpu/isr.h"

void init_syscall();

void syscall_handler(registers_t *r);

