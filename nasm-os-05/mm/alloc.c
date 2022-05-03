#include "./alloc.h"
#include <stddef.h>

#define UNUSED(x) (void)(x)

//内存使用位图
uint8_t *mmap = NULL;

/*
 * 使用map_process存入每一个内存页面所被程序使用情况
 * map_process中存放了使用这个内存页的任务ID所在的
 * 固定内存区域为 [0x200000, 0x600000) 占用4M
 * 对于内核程序所使用的内存页面map_process被设置为0
 * 其它程序所使用的内存页面被设置为其任务ID
 */
uint32_t *map_process = NULL;
uint32_t find_index = MMAP_USED_SIZE;

/*
 * install_alloc : 安装申请内存位图
 * return : void
 */
void install_alloc()
{
	//位图所在的固定内存区域为 [0x100000, 0x200000)
	mmap = (uint8_t *) MMAP;
	for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
		//16M以下均为已使用
		if (i < MMAP_USED_SIZE) {
			//设定内核所占用的1MB内存为已使用
			mmap[i] = (MM_USED | MM_NO_SWAP);
		}
		//剩下的内存为未使用
		else {
			mmap[i] = (MM_FREE | MM_CAN_SWAP);
		}
	}
}

//初始化内存页的使用者
void install_used_map()
{
	map_process = (uint32_t *) MMAP_PRO;
	/*
	 * 初始化map_process，因为map和map_process的大小永远都是一样的所以用MAP_SIZE
	 * map_process所占内存空间为 [0x200000, 0x600000)
	 */
	for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++) {
		map_process[i] = 0;
	}
}

/*
 * alloc_page : 申请内存页，每页为4KB大小
 *  - int count : 页数
 * return : void*返回申请地址，NULL代表申请失败
 */
void* alloc_page(uint32_t process_id, uint32_t count, uint32_t can_swap, uint32_t is_dynamic)
{
  UNUSED(is_dynamic);
  UNUSED(can_swap);
	//查找内存申请地址
	void *ret = NULL;
	//找到空闲内存页计数
	uint32_t num = 0;
	//开始编号
	uint32_t start_with = 0;
	//从未被分配内存页的地方开始查找
	for (uint32_t i = MMAP_USED_SIZE; i < MAP_SIZE; i++)
	{
		//如果找到空闲页
		if ((mmap[i] & 0x1) == MM_FREE)
		{
			//设置可分配内存起始编号
			if (start_with == 0) {
				ret = (void*) (i * MM_PAGE_SIZE);
				start_with = i;
			}
			num++;
		}
		//如果没有找到空闲页
		else {
			//清空变量
			ret = NULL;
			num = 0;
			start_with = 0;
		}
		//找到了可分配内存页，并且找到了预期想要分配的数量
		if (start_with != 0 && num >= count) {
			break;
		}
	}
	//设置map的各个内存页的状态为已使用
	for (uint32_t i = 0; i < count; i++) {
		mmap[start_with + i] = MM_USED | MM_CAN_SWAP; //(MM_USED | ((uint32_t) can_swap << 1) | ((uint32_t) is_dynamic << 2));
		map_process[start_with + i] = process_id;
	}

	//返回查找到内存地址
	return ret;
}

/*
 * alloc_page : 释放内存页，每页为4KB大小
 *  - void *addr : 释放地址
 *  - int count : 释放页数
 * return : void
 */
void free_page(void *addr, uint32_t count)
{
	//释放内存页
	for (uint32_t i = 0; i < count; i++)
	{
		//更新map中这些页的状态
		mmap[(uint32_t) (addr + (i * MM_PAGE_SIZE)) / MM_PAGE_SIZE] = (MM_FREE | MM_CAN_SWAP | MM_NO_DYNAMIC);
		map_process[i] = 0;
	}
}

/*
 * alloc_mm : 申请内存
 *  - int size : 申请大小
 * return : void*返回申请地址，NULL代表申请失败
 */
void* alloc_mm(int size)
{
  int alloc_page_count = (size + 4) / MM_PAGE_SIZE + (size + 4) % MM_PAGE_SIZE > 0 ? 1 : 0;
  void* ptr =  alloc_page(1, alloc_page_count, 0, 0);
  if (ptr != NULL) {
    *((int*)ptr) = alloc_page_count * MM_PAGE_SIZE - 4;
    ptr = (int*)ptr + 1; 
  }
  return ptr; 
}

/*
 * free_mm : 释放内存
 *  - void* addr : 释放地址
 *  - int size : 释放大小
 * return : void
 */
void free_mm(void* addr)
{
  if (addr != NULL) {
    int count = (*(((int*)addr - 1)) + 4)/MM_PAGE_SIZE;
    free_page(addr, count);
  } 
}

uint8_t mmap_status(uint32_t page_no)
{
	return mmap[page_no];
}

uint32_t map_process_id(uint32_t page_no)
{
	return map_process[page_no];
}

void set_mmap_status(uint32_t page_no, uint8_t status)
{
	mmap[page_no] = status;
}

void set_map_process_id(uint32_t page_no, uint32_t pid)
{
	map_process[page_no] = pid;
}

void free_page_by_pid(uint32_t pid)
{
	for (uint32_t i = 0; i < MAP_SIZE_LOGIC; i++)
	{
		if (map_process[i] == pid)
		{
			mmap[i] = (MM_FREE | MM_CAN_SWAP);
			map_process[i] = 0;
		}
	}

}
