#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "threads/init.h"

static void syscall_handler (struct intr_frame *);

static int syswrite(void * esp);
static int sysread(void * esp);
static bool syscreate(void * esp);
static int sysopen(void * esp);
int sysexit(int status);
static tid_t sysexec (void *esp); 
static int syswait (void * esp);
static bool sysremove(void * esp);
static void sysseek(void * esp);
static unsigned systell(void * esp);
static void sysclose(void *esp) ;
static int sysfilesize(void * esp);

static struct fd_file * find_file_by_fd(int fd); 
int is_valid_userptr(void * esp);
int is_valid_physical_addr(void *esp); 


int next_fd;

struct lock filelock; //acquire lock during syswrite


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filelock);
}

static void
syscall_handler (struct intr_frame *f) 
{

  if (!is_valid_userptr(f->esp) || !is_valid_userptr((int *) f->esp + 1)) {
    sysexit(-1); 
  }
  
  // for stack growth purpose
  thread_current()->esp = f->esp;

  int code = *(int *) f->esp;
  
  switch (code) {
    
    case SYS_WRITE: 
    {
      f->eax = syswrite(f->esp);
      break;
    }

    case SYS_READ:
    {
     f->eax = sysread(f->esp);
     break;
    }

    case SYS_CREATE:
    {
      //syscall2 char *file, unsigned initial_size
      f->eax = syscreate(f->esp);
      
      break;
    }

    case SYS_OPEN:
    {
      // syscall1
      f->eax = sysopen(f->esp);
      break;
    }

    case SYS_EXIT:
    {
      int status = *((int *)f->esp +1);
      f->eax = sysexit(status);
      break;
    }

    case SYS_HALT:
    {
      power_off();
      break;
    }
    
    case SYS_EXEC:
    {
      f->eax = sysexec(f->esp);    
      break;
    }

    case SYS_WAIT:
    {
      f->eax = syswait(f->esp);
      break;
    }

    case SYS_FILESIZE:
    {
      f->eax = sysfilesize(f->esp);
      break;
    }

    case SYS_REMOVE:
    {
      f->eax = sysremove(f->esp);
      break;
    }

    case SYS_SEEK:
    {
      sysseek(f->esp);
      break;
    }

    case SYS_TELL:
    {
      f->eax = systell(f->esp);
      break;
    }

    case SYS_CLOSE:
    { 
      sysclose(f->esp);
      break;
    }

  }
}

// switch functions 

static int syswrite (void * esp){ // syscall3
  int fd;
  void * buf;
  size_t size;
  int out;

  get_syscall3_arg(esp, &fd, &buf, &size);

  if (fd == STDOUT_FILENO){
    lock_acquire(&filelock);
    putbuf((char *) buf, size);
    lock_release(&filelock);
    out = size;
  } else {
    lock_acquire(&filelock);
    struct fd_file * f = find_file_by_fd(fd);
    if (f) {
      out = file_write(f->file, buf,size);
      lock_release(&filelock);
    }
    else {
      out = -1;
      lock_release(&filelock);
    }
  }
  return out;
}

static int sysread (void * esp) {
  int fd;
  void * buf;
  unsigned size;
  int out = 0;
  get_syscall3_arg(esp, &fd, &buf, &size);

  if (fd == STDIN_FILENO) { // read from keyboard
    uint8_t * this_buffer = (uint8_t *) buf;
    int i;
    for (i = 0; i<size; i++) {
      this_buffer[i] = input_getc();
    }
    return size;
  } else {
    struct fd_file * read_file = find_file_by_fd(fd);
    if (read_file != NULL) {
      return file_read(read_file->file,  buf, size);
    } else {
      return -1;
    }
  }
}

static int sysopen (void * esp) {
  if (!is_valid_userptr((int *) esp + 1)) {
    sysexit(-1);
  }
  int fd = get_next_fd();
  int out = -1;
  char * fname = * (char **) ((int *) esp + 1);
  if (fname == NULL) sysexit(-1);

  lock_acquire(&filelock);
  struct file * openfile = filesys_open(fname);
  lock_release(&filelock);

  if (openfile == NULL) out = -1;

  else{
    // add the file to the current thread file list
    struct fd_file * new_file = (struct fd_file *) malloc (sizeof(struct fd_file));
    new_file->fd = fd;
    new_file->file = openfile;
    list_push_back(&thread_current()->file_list, &new_file->elem);
    out = fd;
  }

  return out;

}
  

static bool syscreate(void *esp) {
  if (!is_valid_userptr((int *) esp + 1) || !is_valid_userptr((int *) esp + 2)) sysexit(-1);
  char *file_name = *(char **) ((int *) esp+1);
  if (file_name == NULL) sysexit(-1);

  unsigned init_size = *((int *) esp + 2);
  lock_acquire(&filelock);
  bool success =  filesys_create(file_name, init_size);
  lock_release(&filelock);

  return success;
}

int sysexit(int status) {

  // child exit -> notify parent
  struct child_process_pid * c = find_child_process_pid(thread_current()->tid, thread_current()->parent);
  c->exit_status = status;
  c->wait_success = 1;
  thread_current()->exit_status = status; // update exit_status of this exiting thread

  if (thread_current()->parent->waiting_child == thread_current()->tid)
    sema_up(&thread_current()->parent->child_lock);

  printf("%s: exit(%d)\n", thread_current()->name, status);
  // close all the file_list files
  /*
  if (!list_empty(&thread_current()->file_list)) {
    struct list_elem *e;
    for (e=list_begin(&thread_current()->file_list); e != list_end(&thread_current()->file_list); e=list_next(e)) {
      struct fd_file *f = list_entry(e, struct fd_file, elem);
      file_close(f->fd);
    }
  }
  */

  thread_exit();
  return status;
}

static tid_t sysexec (void *esp) {
  if (!is_valid_userptr((int *) esp + 1)) sysexit(-1);
  char *file_name = *(char **) ((int *) esp + 1);

  lock_acquire(&filelock);
  tid_t child_pid = process_execute(file_name); //returns TID_ERROR if thread not created 
  lock_release(&filelock);


  return child_pid;

}

static int syswait(void *esp) {
  int pid = *((int *) esp + 1);
  int return_status;
  return_status = process_wait(pid);
  //printf("syswait pid %d\n", pid);
  return return_status;
     
}

static bool sysremove(void * esp){
  char *file_name = *(char **) ((int *) esp +1);
  bool success;
  lock_acquire(&filelock);
  success = filesys_remove(file_name);
  lock_release(&filelock);
  return success;
}

static void sysseek(void * esp) {
  if (!is_valid_userptr((int *) esp + 2)) {
    sysexit(-1);
  }
  int fd = *((int *) esp + 1);
  unsigned pos = *(unsigned **) ((int *) esp + 2);

  struct fd_file * f = find_file_by_fd(fd);
  if (f) {
    lock_acquire(&filelock);
    file_seek(f->file, pos);
    lock_release(&filelock);
  }
}

static unsigned systell(void * esp) {
  int fd = *((int *) esp + 1);
  int pos = 0;
  struct fd_file * f = find_file_by_fd(fd);
  if(f)
    pos = file_tell(f->file);
  return pos;
}


static void sysclose(void * esp) {
  int fd = *((int *) esp +1 );
  struct fd_file * removing_file = find_file_by_fd(fd);
  if(removing_file) {
    file_close(removing_file->file);
    list_remove(&removing_file->elem);
    free(removing_file);
  }
}

static int sysfilesize(void * esp) {
  int fd = *((int *) esp + 1);
  struct fd_file * f = find_file_by_fd(fd);
  if (f) {
    return file_length(f->file);
  }
  return -1;
}


// additional functions


// get arguments from esp, number depends on the type of syscall

void get_syscall3_arg (void *esp, int *arg1, void **arg2, unsigned * arg3){
    if (!is_valid_userptr((int *) esp + 1) || !is_valid_userptr((int *) esp + 2) || !is_valid_userptr((int *) esp + 3)) sysexit(-1);
    *arg1 = *((int *) esp+1);
    *arg2 =  *(void **) ((int *) esp +2);
    *arg3 =  *(unsigned *)((int *) esp + 3);
}

// check if pointer is valid within the user memory range
int is_valid_userptr(void *esp) {
  if (esp < (void *) 0x08048000 || !is_user_vaddr(esp)) {
   // printf("invalid pointer\n");
    return 0;
  }
  if (!is_valid_physical_addr(esp)) return 0;
  return 1;

}

// do i need to do this??
int is_valid_physical_addr(void *esp) {
  void * p = pagedir_get_page(thread_current()->pagedir, esp);
  if (!p) 
    return 0;
  return 1;
}


// find the file using file descriptor value
static struct fd_file * find_file_by_fd(int fd) {
  struct list_elem *e;
  for (e= list_begin(&thread_current()->file_list); e != list_end(&thread_current()->file_list); e=list_next(e)) {
    struct fd_file *f = list_entry(e, struct fd_file, elem);
    if (f->fd == fd) {
      return f;
    }
  }
  return NULL;
}

int get_next_fd() {
  next_fd +=1;
  return next_fd;
}
