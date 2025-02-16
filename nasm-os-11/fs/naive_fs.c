#include "fs/naive_fs.h"
#include "fs/vfs.h"
#include "drivers/hd.h"
#include "libc/string.h"
#include "libc/kprint.h"
#include "cpu/x86.h"
#include "mm/alloc.h"
#include <stddef.h>

fs_t naive_fs;
uint32_t file_num;
naive_file_meta_t file_metas[100];

fs_t* get_naive_fs()
{
  return &naive_fs;
}

static int32_t naive_fs_stat_file(char* filename, file_stat_t* stat)
{
  kprintf("naive_fs_stat_file: %x %s\n", file_num, filename);
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    if (strcmpN(meta->filename, filename, sizeof(meta->filename)) == 0) {
      stat->size = meta->size;
      return 0;
    }
  }
  return -1;
}

static int32_t naive_fs_list_dir(char* dir)
{
  UNUSED(dir);
  uint32_t size_length[file_num];
  uint32_t max_length = 0;
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    uint32_t size = meta->size;
    uint32_t length = 1;
    while ((size /= 10) > 0) {
      length++;
    }
    size_length[i] = length;
    max_length = length > max_length ? length : max_length;
  }

  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    kprintf("root  ");
    for (uint32_t j = 0; j < max_length - size_length[i]; j++) {
      kprintf(" ");
    }
    kprintf("%d  %s\n", meta->size, meta->filename);
  }
  return -1;
}

static int32_t naive_fs_read_data(char* filename, char* buf, uint32_t start, uint32_t length)
{
  kprintf("naive_fs_read_data>>>>>>>>>> %s %d\n", filename, sizeof(file_metas[0].filename));
  naive_file_meta_t* file_meta = NULL;
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    if (strcmpN(meta->filename, filename, sizeof(meta->filename)) == 0) {
      file_meta = meta;
      break;
    }
  }
  if (file_meta == NULL) {
    return -1;
  }

  uint32_t offset = file_meta->offset;
  uint32_t size = file_meta->size;
  if (length > size) {
    length = size;
  }
  reset_hd_controller();
  read_hd_split(false, buf, naive_fs.partition.offset + offset + start, length);
  kprint_hex_n(buf, 20);
  kprintf("\n");
  kprint_hex_n(buf+4096, 20);
  kprintf("\n");
  kprint_hex_n(buf+8192, 20);
  kprintf("\n");
  return length;
}

static int32_t naive_fs_write_data(char* filename, char* buf, uint32_t offset, uint32_t length)
{
  UNUSED(filename);
  UNUSED(buf);
  UNUSED(offset);
  UNUSED(length);
  return 0;
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
  read_hd(false, (char*)file_metas, 4 + naive_fs.partition.offset, meta_size);
  for (uint32_t i = 0; i < file_num; i++) {
    naive_file_meta_t* meta = file_metas + i;
    kprintf("file name=%s offset=%d size=%d\n", meta->filename, meta->offset, meta->size);
  }
}

