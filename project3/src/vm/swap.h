#ifndef VM_SWAP_H
#define VM_SWAP_H


#include "devices/disk.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"

struct swap_slot {
  struct list_elem elem;
  bool free;
  size_t index;
};

struct disk * swap_disk;
struct list swap_table;
struct lock swap_table_lock;

void swap_table_init(void);
void swap_into_physical_mem (size_t index, void * user_page_addr);
size_t swap_out_to_disk(void * user_page_addr);

#endif
