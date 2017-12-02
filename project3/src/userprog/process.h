#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct fd_file {
  int fd;
  struct file * file;
  struct list_elem open_elem;
  struct list_elem elem;
};

struct child_process_pid {
  int pid;
  int exit_status;
  int wait_success;
  struct list_elem elem;
};

struct child_process_pid * find_child_process_pid(int pid, struct thread * t); 
bool install_page(void *upage, void *kpage, bool writable);

extern int next_fd;  // need to be initialized to 3 in process.c? or somewhere else?

#endif /* userprog/process.h */
