#pragma once

#include <stdint.h>

struct file_stat {
  uint32_t size;
  uint8_t acl;
};
typedef struct file_stat file_stat_t;

