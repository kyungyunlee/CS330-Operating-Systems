#include "vm/swap.h"
#include "devices/disk.h"
#include "lib/kernel/list.h"
#include "threads/vaddr.h"

void swap_table_init(){
  // create a swap table with free swap slots
  // DISK_SECTOR_SIZE == 512
  list_init(&swap_table);
  lock_init(&swap_table_lock);

  // create a swap disk ***global struct disk *  
  swap_disk = disk_get(1,1);
  if (swap_disk == NULL) return;
  disk_sector_t diskSize = disk_size(swap_disk);
  // create swap table
  int num_entry = diskSize / (PGSIZE/DISK_SECTOR_SIZE);
  int i;
  for (i=0;  i<num_entry;  i++) {
    struct swap_slot * ss = (struct swap_slot *) malloc(sizeof(struct swap_slot));
    if (ss == NULL) return;
    ss->free = true;
    ss->index = i+1;
    list_push_back(&swap_table, &ss->elem);
  }
}

void swap_into_physical_mem(size_t index, void * user_page_addr){
    struct list_elem * e = list_begin(&swap_table);
    struct swap_slot * ss;
    bool swap_slot_found = false;
    lock_acquire(&swap_table_lock);
    for (e; e!= list_end(&swap_table); e=list_next(e)){
        ss = list_entry(e, struct swap_slot, elem);
        if (ss->index == index && ss->free == false) {
            ss->free = true;
            swap_slot_found = true;
            break;
        }
    }
    if (!swap_slot_found) {
        lock_release(&swap_table_lock);
        return;
    }

    int i;
    int num_swap_slots = PGSIZE/DISK_SECTOR_SIZE;
    for (i=0;i<num_swap_slots;i++) {
        disk_read(swap_disk, index * num_swap_slots + i, (uint8_t *) user_page_addr + i * DISK_SECTOR_SIZE);
    }
    
    lock_release(&swap_table_lock);
}

size_t swap_out_to_disk(void * user_page_addr) {
    if(!is_user_vaddr(user_page_addr)) return -1;
  bool swap_slot_found = false;
  size_t swap_slot_index;

  lock_acquire(&swap_table_lock);
  // find available swap slot
  struct list_elem * e ;
  for (e = list_begin(&swap_table); e != list_end(&swap_table); e = list_next(e)){
    struct swap_slot * ss = list_entry(e, struct swap_slot, elem);
    if (ss->free) {
      ss->free = false;
      swap_slot_index = ss->index;
      swap_slot_found = true;
      break;
    }
  }

  if (!swap_slot_found) {
    lock_release(&swap_table_lock);
    return -1;
  }
  // disk_write
  int i;
  int num_swap_slots = PGSIZE/DISK_SECTOR_SIZE;
  for (i=0; i < num_swap_slots;i++) { 
     disk_write(swap_disk, swap_slot_index * num_swap_slots + i, (uint8_t * ) user_page_addr + i * DISK_SECTOR_SIZE); 
  }

  lock_release(&swap_table_lock);
  return swap_slot_index;
}




