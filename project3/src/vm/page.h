#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdint.h>
#include "filesys/off_t.h"
#include "threads/synch.h"
#include "lib/kernel/hash.h"
#include "threads/thread.h"

struct page {
  struct hash_elem hash_elem;
  // where is this page?
  int data_location; // 1 : file , 2 : swap , 3 : zero, 4: on frame/physical memory

  //file 
  struct file * file;
  off_t ofs;
  uint8_t * upage; // virtual page address
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;

  void * kpage;

//  bool is_loaded; // whether the page is in the physical memory or not

  // swap slot
  size_t swap_slot_index; // if page is swapped out to swap_slot, save index
  
  struct thread * owner;
};

void supp_page_table_init(struct thread * t);
bool add_file_supp_page_table_entry(struct thread *t, struct file * file, off_t ofs, uint8_t * upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
bool add_swap_supp_page_table_entry(struct thread *t, uint8_t * upage);
bool add_frame_supp_page_table_entry(struct thread * t, uint8_t * upage, uint8_t * kpage);
struct page * create_and_return_pte(struct thread * t, uint8_t * upage);
struct page * get_supp_page_entry(struct thread * t, void * page_addr);

bool load_file(struct page * p);
bool load_swap(struct page * p);




#endif
