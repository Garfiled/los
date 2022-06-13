#pragma once

#include "fs/file.h"

enum fs_type {
  NAIVE,
  EXT4
};

struct disk_partition {
  uint32_t offset;
};

typedef struct disk_partition disk_partition_t;

typedef int32_t (*stat_file_func)(char* filename, file_stat_t* stat);
typedef int32_t (*list_dir_func)(char* dir);
typedef int32_t (*read_data_func)(char* filename, char* buffer, uint32_t start, uint32_t length);
typedef int32_t (*write_data_func)(char* filename, char* buffer, uint32_t start, uint32_t length);

struct file_system {
  enum fs_type type;
  disk_partition_t partition;

  // functions
  stat_file_func stat_file;
  list_dir_func list_dir;
  read_data_func read_data;
  write_data_func write_data;
};
typedef struct file_system fs_t;


// ****************************************************************************
void init_file_system();

int32_t stat_file(char* filename, file_stat_t* stat);
int32_t list_dir(char* dir);
int32_t read_file(char* filename, char* buffer, uint32_t start, uint32_t length);
int32_t write_file(char* filename, char* buffer, uint32_t start, uint32_t length);

