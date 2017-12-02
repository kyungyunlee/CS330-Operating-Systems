#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"

//extern struct list frame_table;
//extern struct lock frame_table_lock; 

struct frame {
    struct list_elem elem;
    void * page_ptr;
    struct thread * owner;
    void * frame_ptr;
    bool pinned; // 
};

void frame_table_init(void);
void * allocate_new_frame(enum palloc_flags flags, void * upage);
void deallocate_frame(void * kpage);

#endif
