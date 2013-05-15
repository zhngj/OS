#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "opt-A2.h"
#include <types.h>
#include <machine/trapframe.h>
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */



int sys_reboot(int code);
#if OPT_A2
int sys_open(const char* filename,int flags,int *err);
int sys_close(int fd,int *err);
int sys_read(int fd, void *buf, size_t buflen,int *err);
int sys_write(int fd, const void *buf, size_t nbytes,int *err);
void sys__exit(int code);
pid_t sys_fork(struct trapframe* tf, int *err);
pid_t sys_waitpid(pid_t pid,int* status, int option,int* err);
pid_t sys_getpid(void);
int sys_execv(const char* prog, char** args,int* err);
//int execv(const char* prog, char** args,int *err);

#endif

#endif /* _SYSCALL_H_ */
