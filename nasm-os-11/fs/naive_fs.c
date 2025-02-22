#include "fs/naive_fs.h"
#include "fs/vfs.h"
#include "drivers/hd.h"
#include "libc/kprint.h"
#include "cpu/x86.h"
#include <stddef.h>
#include "libc/kprint.h"
#include "libc/string.h"

fs_t naive_fs;
uint32_t file_num;
naive_file_meta_t file_metas[100];

fs_t* get_naive_fs()
{
  return &naive_fs;
}

static naive_file_meta_t* find_file_meta(const char* filename)
{
  for (uint32_t i = 0; i < file_num; i++) {
      naive_file_meta_t* meta = &file_metas[i];
      if (strcmp(meta->filename, filename) == 0) {
          return meta;
      }
  }
  return NULL;
}

static int32_t naive_fs_stat_file(const char* filename, file_stat_t* stat)
{
  kprintf("naive_fs_stat_file: %x %s\n", file_num, filename);
  naive_file_meta_t* file_meta = find_file_meta(filename);
  if (file_meta != NULL) {
	stat->size = file_meta->size;
	return 0;
  }
  return -1;
}

static int32_t naive_fs_list_dir(const char* dir)
{
  UNUSED(dir);
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
	kprintf("%s %d\n", meta->filename, meta->size);
  }
  return 0;
}

static int32_t naive_fs_read_data(const char* filename, char* buf, uint32_t start, uint32_t length)
{
  naive_file_meta_t* file_meta = find_file_meta(filename);
  if (file_meta == NULL) {
    return -1;
  }

  uint32_t offset = file_meta->offset;
  uint32_t size = file_meta->size;
  if (length > size) {
    length = size;
  }
  uint32_t device_addr = naive_fs.partition.offset + offset + start;
  read_hd_split(false, buf, device_addr, length);
  return length;
}

static int32_t naive_fs_write_data(const char* filename, char* buf, uint32_t offset, uint32_t length)
{
  naive_file_meta_t* file_meta = find_file_meta(filename);
  if (!file_meta) {
	kprintf("file not exist:%s\n", filename);
    return -1; // 文件不存在
  }

  if (offset + length > file_meta->size) {
	kprintf("offset and length overflow:%d %d\n", offset, length);
    return -1; // 超出文件范围
  }
  write_hd_split(false, buf, naive_fs.partition.offset + file_meta->offset + offset, length);
  return length;
}

void init_naive_fs()
{
  kprintf("init_naive_fs\n");
  naive_fs.type = NAIVE;
  // second device 0KB
  naive_fs.partition.offset = 0;

  naive_fs.stat_file = naive_fs_stat_file;
  naive_fs.read_data = naive_fs_read_data;
  naive_fs.write_data = naive_fs_write_data;
  naive_fs.list_dir = naive_fs_list_dir;

  read_hd(false, (char*)&file_num, 0 + naive_fs.partition.offset, sizeof(uint32_t));
  kprintf("file_num:%d %x\n", file_num, file_num);
  // 不知道这里为什么需要reset HD controller, 否则读取出来的值都是0
  reset_hd_controller();

  uint32_t meta_size = file_num * sizeof(naive_file_meta_t);
  // 如果通过alloc_mm申请的话，其他进程看不到
  //file_metas = (naive_file_meta_t*)alloc_mm(meta_size);
  read_hd(false, (char*)file_metas, sizeof(uint32_t) + naive_fs.partition.offset, meta_size);
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    kprintf("file name=%s offset=%d size=%d\n", meta->filename, meta->offset, meta->size);
  }
}

