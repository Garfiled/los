#pragma once

#include "fs/file.h"

struct naive_file_meta 
{
  char filename[4];
  uint32_t size;
  uint32_t offset;
};

typedef struct naive_file_meta naive_file_meta_t;

void init_naive_fs();
