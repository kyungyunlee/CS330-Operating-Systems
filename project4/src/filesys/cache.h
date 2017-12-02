#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/disk.h"
#include "threads/synch.h"
#include <string.h>
#include "threads/malloc.h"
#include <list.h>


#define BUFFER_CACHE_SIZE 64

struct buffer_cache_entry {
  struct list_elem elem;

  bool is_dirty;
  bool is_accessed;
  
  disk_sector_t sector_num;
  uint8_t buffer[DISK_SECTOR_SIZE]; // 512 bytes 
};

struct list buffer_cache;
struct lock buffer_cache_lock;

void buffer_cache_init(void);
void buffer_cache_read(struct disk*, disk_sector_t, void*);
void buffer_cache_write(struct disk*, disk_sector_t, const void*);
#endif
