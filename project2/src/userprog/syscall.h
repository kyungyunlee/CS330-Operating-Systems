#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

int sysexit(int status);

int get_next_fd();


#endif /* userprog/syscall.h */
