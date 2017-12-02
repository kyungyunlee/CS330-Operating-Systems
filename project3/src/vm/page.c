#include "vm/page.h"
#include <stdio.h>
#include <stdlib.h>
#include "filesys/file.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "lib/kernel/hash.h"
#include "userprog/pagedir.h"

#include "vm/frame.h"
#include "vm/swap.h"

unsigned page_hash(const struct hash_elem * p_, void *aux UNUSED);
bool page_less(const struct hash_elem * a_, const struct hash_elem * b_, void *aux UNUSED);

void supp_page_table_init(struct thread * t){
    hash_init(&t->supp_page_table, page_hash, page_less, NULL);
//  lock_init(&spt);
}

bool add_file_supp_page_table_entry(struct thread *t, struct file * file, off_t ofs, uint8_t * upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable){
  struct page * p = (struct page *) malloc(sizeof(struct page));
  if (p == NULL) return false;
  p->file = file;
  p->ofs = ofs;
  p->upage = upage;
  p->read_bytes = read_bytes;
  p->zero_bytes = zero_bytes;
  p->writable = writable;
  p->data_location = 1; // file
//  p->is_loaded = false;
  p->owner = t;

  // insert to the current thread's supplemental page table
  struct hash_elem * h = hash_insert(&t->supp_page_table, &p->hash_elem);
  if (h== NULL) return true;

  free(p);
  return false;

}

bool add_swap_supp_page_table_entry(struct thread *t, uint8_t * kpage) {
  struct page * p = (struct page *) malloc(sizeof (struct page));
  if (p== NULL) return false;
 // p->is_loaded =true;
  p->data_location = 2; // swap
  p->writable = true;
  p->owner = t;
  p->kpage = kpage;
  struct hash_elem * h = hash_insert(&t->supp_page_table, &p->hash_elem);
  if (h== NULL) return true;
  free(p);
  return false;
}

/*
bool add_frame_supp_page_table_entry(struct thread * t, uint8_t * upage, uint8_t * kpage) {
  struct page * p = (struct page *) malloc (sizeof(struct page));
  if (p == NULL) return false;
  p->data_location = 4; // loaded to frame
  p->owner = t;
  p->upage = upage;
  p->kpage = kpage;
  
  struct hash_elem * h = hash_insert(&t->supp_page_table, &p->hash_elem);
  if (h== NULL) return true;
  free(p);
  return false;
}
*/


void remove_supp_page_table_entry(struct thread * t,struct page * p){
  struct hash * spt = &t->supp_page_table;
  hash_delete(spt, &p->hash_elem);
}

// get the page table entry of the given page addr
struct page * get_supp_page_entry(struct thread * t, void * page_addr) {
  struct hash * spt_ptr = &t->supp_page_table;
  struct page p;
  struct hash_elem *e;
  p.upage = page_addr;
  e = hash_find(spt_ptr, &p.hash_elem);
  if (e == NULL) {
    return NULL;
  }
  return hash_entry(e, struct page, hash_elem) ;
}

bool load_file(struct page * p){
  uint32_t read_bytes = 0;
  bool success = false;
  enum palloc_flags flags = PAL_USER;
  /*
  if (p->read_bytes == 0) {
    // zero it
    flags = PAL_USER | PAL_ZERO;
  }
  */
  if (p != NULL) {
    // obtain frame to store the page
    void * f = allocate_new_frame(flags, p->upage);
    if (f == NULL){ 
      return false;
    }
    file_seek(p->file, p->ofs);
    if (p->read_bytes >0 ) {
      read_bytes = file_read_at(p->file, f, p->read_bytes, p->ofs);
      if (read_bytes != p->read_bytes) {
        // didnt fully read as desired -> freeeeee
        deallocate_frame(f);
        return false;
      }
      // fetch the data into frame
      memset(f + p->read_bytes, 0, p->zero_bytes);
    }
    
    // point the page table entry for the faulting virtual address to the physical page
//    success = install_page(p->upage, f, p->writable) ;
    success  = pagedir_set_page(thread_current()->pagedir, p->upage, f, p->writable);
    if (!success ){
      deallocate_frame(f);
      return false;
    }
//  p->is_loaded = true;
  p->data_location = 4; // data in frame
  return true;
  }
}

bool load_swap(struct page * p){
    uint8_t * f = allocate_new_frame(PAL_USER, p->upage);
    if (f==NULL) return false;
    swap_into_physical_mem(p->swap_slot_index, f);
    if (!pagedir_set_page(thread_current()->pagedir, (void *) p->upage, (void *) f, p->writable)){
        deallocate_frame(f);
        return false;
    }
    p->data_location = 4;
    return true;
}


bool grow_stack (void * fault_page_addr) {
  // create a new page table entry
  struct page * p = (struct page *) malloc (sizeof(struct page));
  if (p == NULL) return false;
  p->upage = fault_page_addr;
 // p->is_loaded = true;
  p->data_location = 3;
  p->writable = true;

  void * f = allocate_new_frame (PAL_ZERO | PAL_USER, p->upage);
  if (f == NULL) {
    return false;
  }

  bool success  = pagedir_set_page(thread_current()->pagedir, fault_page_addr, f, p->writable);
  if (!success) {
    deallocate_frame(f);
    free(p);
    return false;
  }
  // allocate a new frame for this new page
  // install page
 // point the page table entry for the faulting virtual address to the physical page
 
  struct hash_elem * h = hash_insert(&thread_current()->supp_page_table, &p->hash_elem);
  if (h== NULL) return true;
  return false;
 
}
    

// hash functions 
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED) {
  const struct page * p = hash_entry(p_, struct page, hash_elem);
  return hash_bytes(&p->upage, sizeof p->upage);
}

bool page_less(const struct hash_elem *a_, const struct hash_elem * b_, void * aux UNUSED) {
  const struct page *a = hash_entry(a_, struct page, hash_elem);
  const struct page *b = hash_entry(b_, struct page, hash_elem);
  return a->upage < b->upage;
}





