#include "mm/alloc.h"
#include <stddef.h>
#include <stdint.h>

const uint32_t heap_start_addr = 20 * 1024 * 1024;
uint32_t heap_curr_addr = heap_start_addr;

void *alloc_mm(int size)
{
  // find fit
  uint32_t addr = heap_start_addr;
  void *alloc_addr = NULL;
  mm_desc *find_mm = NULL;
  while (addr < heap_curr_addr) {
    find_mm = (mm_desc*)addr;
  	if (find_mm->free_ && find_mm->len_ >= size && find_mm->len_ / 2 < size) {
      find_mm->free_ = false;
  	  alloc_addr = (void*)(addr + sizeof(mm_desc));
  	  break;
  	} else {
  	  addr += sizeof(mm_desc) + find_mm->len_;
  	}
  }
  if (alloc_addr != NULL) {
    return alloc_addr;
  }
  mm_desc *mm = (mm_desc*)heap_curr_addr;
  mm->free_ = false;
  mm->len_ = size;
  mm->prev_ = find_mm;
  alloc_addr = (void*)(heap_curr_addr + sizeof(mm_desc));
  heap_curr_addr += sizeof(mm_desc) + size;
  return alloc_addr;
}

void* alloc_mm_align(int size)
{
  if (size % ALIGN_MEM_SIZE != 0) {
    size = (size / ALIGN_MEM_SIZE + 1) * ALIGN_MEM_SIZE;
  }
	uint32_t addr = heap_start_addr;
	void *alloc_addr = NULL;
	mm_desc *find_mm = NULL;
	while (addr < heap_curr_addr) {
	  find_mm = (mm_desc*)addr;
		if (find_mm->free_ && find_mm->len_ >= size && find_mm->len_ / 2 < size
										&& (addr + sizeof(mm_desc)) % ALIGN_MEM_SIZE == 0) {
      find_mm->free_ = false;
			alloc_addr = (void*)(addr + sizeof(mm_desc));
		  break;
		} else {
		  addr += sizeof(mm_desc) + find_mm->len_;
		}
	}
	if (alloc_addr != NULL) {
	  return alloc_addr;
	}

	uint32_t padding_size = (heap_curr_addr + sizeof(mm_desc)) % ALIGN_MEM_SIZE;
  if (padding_size == 0) {
	} else if (padding_size < sizeof(mm_desc)) {
	  padding_size += ALIGN_MEM_SIZE;
	}
	if (padding_size > 0) {
	  mm_desc *mm_padding = (mm_desc*)heap_curr_addr;
	  mm_padding->free_ = true;
    mm_padding->len_ = padding_size - sizeof(mm_desc);
	  mm_padding->prev_ = find_mm;
	  heap_curr_addr += padding_size;
		find_mm = mm_padding;
  }

	mm_desc *mm = (mm_desc*)heap_curr_addr;
	mm->free_ = false;
	mm->len_ = size;
	mm->prev_ = find_mm;
	alloc_addr = (void*)(heap_curr_addr + sizeof(mm_desc));
	heap_curr_addr += sizeof(mm_desc) + size;
	return alloc_addr;
}

void free_mm(void* addr)
{
  mm_desc *mm = (mm_desc*)((char*)addr - sizeof(mm_desc));
	mm->free_ = true;
	// TO merge prev
	mm_desc *prev = mm->prev_;
	while (prev != NULL) {
	  if (prev->free_) {
		  prev->len_ += sizeof(mm_desc) + mm->len_;
			mm = prev;
			prev = mm->prev_;
		} else {
		  break;
		}
	}
	// TO merge next
  uint32_t next = (uint32_t)mm + sizeof(mm_desc) + mm->len_;
	while (next < heap_curr_addr) {
	  mm_desc *mm_next = (mm_desc*)next;
    if (mm_next->free_) {
	    mm->len_ += sizeof(mm_desc) + mm_next->len_;
			next = (uint32_t)mm + sizeof(mm_desc) + mm->len_;
		  if (next < heap_curr_addr) {
			  mm_next = (mm_desc*)next;
				mm_next->prev_ = mm;
			}
		} else {
		  break;
		}
	}

	if ((uint32_t)mm + mm->len_ + sizeof(mm_desc) == heap_curr_addr) {
	  heap_curr_addr = (uint32_t)mm;
	}
}
