#include "vm/frame.h"
#include <stdio.h>
#include <stdlib.h>
#include "filesys/file.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "lib/kernel/list.h"
#include "lib/string.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "vm/swap.h"


//static void * evict_frame();
static void add_frame_to_table(void * page_ptr, void * upage);
static bool evict_frame(void);
struct frame * get_victim_frame(void);

struct lock frame_table_lock;
struct list frame_table;

void frame_table_init(){
  list_init(&frame_table);
  lock_init(&frame_table_lock);
}

// wrapper for palloc_get_page to handle null page pointer
void * allocate_new_frame(enum palloc_flags flags, void * upage) {
  // page_ptr is kernel virtual address of a single free page 
  void * frame_ptr = palloc_get_page(flags);
  if (frame_ptr != NULL) {
    add_frame_to_table(frame_ptr, upage);
    return frame_ptr;
  }
  else {
    // swap
      bool success = evict_frame();
      if (!success) {
        return NULL;
      }
      
      uint8_t * new_frame_ptr = palloc_get_page(flags);

      if (new_frame_ptr != NULL) {
          add_frame_to_table((void *) new_frame_ptr, upage);
      }

      return new_frame_ptr;
      //return NULL;
  } 
}

// create a frame and add to the frame table
static void add_frame_to_table(void * frame_ptr, void * upage){
  struct frame *f = malloc(sizeof(struct frame)); 
  if (f == NULL) return NULL;
  f->owner = thread_current();
  f->page_ptr=  upage;
  f->frame_ptr = frame_ptr;
  
  // insert frame into the frame table
  lock_acquire(&frame_table_lock);
  list_push_back(&frame_table, &f->elem);
  lock_release(&frame_table_lock);
}

void deallocate_frame(void * kpage) {
  // find the frame in the frame table
  // free the struct frame
  // remove from the frame table list

  struct frame * f;
  struct list_elem * e;
  lock_acquire(&frame_table_lock);
  for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) { 
     f = list_entry(e, struct frame, elem);
     if (f->page_ptr == kpage) {
      list_remove(e);
      palloc_free_page(kpage);
      free(e);
      break;
     }
  }
  lock_release(&frame_table_lock);
}


static bool evict_frame(void){
  // find victim page in the physical memory
  // second chance algorithm
  lock_acquire(&frame_table_lock);
  struct frame * victim_frame = get_victim_frame();
  if (victim_frame == NULL) return false;
  
  // swap out this victim page to swap disk -> swap_out
  struct page * p = get_supp_page_entry(victim_frame->owner, victim_frame->page_ptr);
  if (p == NULL) {
    // make a new entry
    return false;
  }

  // if page has been modified 
  if (pagedir_is_dirty(victim_frame->owner->pagedir, victim_frame->page_ptr)) {
    p->swap_slot_index= swap_out_to_disk(p->upage);
    p->data_location = 2; // is in swap
  }

  memset(victim_frame->frame_ptr, 0, PGSIZE);
//  p->is_loaded = false; // is not in physical memory anymore
  p->data_location = 2;
  list_remove(&victim_frame->elem);

  pagedir_clear_page(victim_frame->owner->pagedir, victim_frame->page_ptr);
  palloc_free_page(victim_frame->frame_ptr);
  free(victim_frame);

  lock_release(&frame_table_lock);
  return true;
}

struct frame * get_victim_frame(void){
  struct frame * f;
  struct list_elem * e ;
  e = list_begin(&frame_table);
//  for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
  while (true) { 
    f = list_entry(e, struct frame, elem);
    if (pagedir_is_accessed(f->owner->pagedir, f->page_ptr)) {
      pagedir_set_accessed(f->owner->pagedir, f->page_ptr, false);
    } else {
      return f;
    }
    e = list_next(&frame_table);
    if (e == list_end(&frame_table)) {
      e = list_begin(&frame_table);
    } 
  }
}


/*
static void * get_frame(){
  return NULL;
}
*/


