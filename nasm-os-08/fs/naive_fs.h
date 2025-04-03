#pragma once

#include "fs/vfs.h"

struct naive_file_meta
{
  char filename[4];
  uint32_t size;
  uint32_t offset;
};

typedef struct naive_file_meta naive_file_meta_t;

void init_naive_fs();
fs_t* get_naive_fs();
