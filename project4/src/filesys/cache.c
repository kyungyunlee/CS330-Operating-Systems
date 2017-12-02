#include "cache.h"
#include <stdio.h>

void insert_into_cache(struct disk*, disk_sector_t, void*, bool);
void evict_cache_entry (struct disk *);
struct buffer_cache_entry * search_buffer_cache_entry (disk_sector_t);
bool is_cache_full (void);


int num_cache_entry = 0;

void buffer_cache_init(void) {
  list_init(&buffer_cache);
  lock_init(&buffer_cache_lock);
}

/* wrapper for disk_read */
void buffer_cache_read (struct disk *d, disk_sector_t sec_no, void *buffer){
  lock_acquire(&buffer_cache_lock);
 // find the entry in the cache
 struct buffer_cache_entry * bce = search_buffer_cache_entry(sec_no);
 // if hit, read from cache
 if (bce) { 
   memcpy(buffer, bce->buffer, DISK_SECTOR_SIZE);
   bce->is_accessed = 1;
 }
 // if miss, insert data into cache
 else {
    insert_into_cache(d, sec_no, buffer, 0);
 }
 lock_release(&buffer_cache_lock);
}

void insert_into_cache (struct disk*d, disk_sector_t sec_no, void *buffer, bool is_write) {
  struct buffer_cache_entry * bce = (struct buffer_cache_entry *) malloc(sizeof(struct buffer_cache_entry));
  bce->is_accessed = 1;
  bce->sector_num = sec_no;
  
  if (is_write) {
    memcpy(bce->buffer, buffer, DISK_SECTOR_SIZE);
    bce->is_dirty = 1;
  } 
  else {
    disk_read(d, sec_no, buffer); // read from disk, put it into buffer
    memcpy(bce->buffer, buffer, DISK_SECTOR_SIZE); // copy buffer data into cache buffer
  }
  
  if (!is_cache_full()){
    // insert into cache
    list_push_front(&buffer_cache, &bce->elem);
    num_cache_entry ++;
  } else {
    // evict cache
    evict_cache_entry(d);
    list_push_front(&buffer_cache, &bce->elem);
  }
}

/* clock algorithm */
void evict_cache_entry (struct disk * d){
  bool victim_found = false;
  struct list_elem * e = list_begin(&buffer_cache);
  struct buffer_cache_entry * victim_bce;
  while (!victim_found) {
    victim_bce = list_entry(e, struct buffer_cache_entry, elem);
    if (victim_bce->is_accessed == 0) {
      victim_found = true;
    }
    else {
      victim_bce->is_accessed = 0;
    }
    e = list_next(e);
    if (e == list_end(&buffer_cache)){
      e = list_begin(&buffer_cache);
    }
  }
  
  /* need to write back to disk */
  if (victim_bce->is_dirty) {
    disk_write(d, victim_bce->sector_num, victim_bce->buffer);
  }
  list_remove(&victim_bce->elem);
  free(victim_bce);
}

/* wrapper for disk_write */
void buffer_cache_write (struct disk *d, disk_sector_t sec_no, const void * buffer) {
  lock_acquire(&buffer_cache_lock);
  struct buffer_cache_entry * bce = search_buffer_cache_entry(sec_no);
  if (bce) {
    // just write to cache
    memcpy(bce->buffer, buffer, DISK_SECTOR_SIZE);
    bce->is_accessed = 1;
    bce->is_dirty = 1;
  } 
  else {
    // bring the stuff into cache
    // update with new data
    insert_into_cache(d, sec_no, buffer, 1);
  }

  lock_release(&buffer_cache_lock);
}

struct buffer_cache_entry * search_buffer_cache_entry (disk_sector_t sec_no){
  struct list_elem *e = list_begin(&buffer_cache);
  for (e ; e != list_end(&buffer_cache); e = list_next(e)){
    struct buffer_cache_entry * bce = list_entry(e, struct buffer_cache_entry, elem);
    if (bce->sector_num == sec_no) {
      return bce;
    }
  }
  return NULL;
}

/* Returns true if cache is full */
bool is_cache_full (void) {
  return num_cache_entry >= BUFFER_CACHE_SIZE;
}
