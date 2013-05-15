#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <thread.h>
#include <curthread.h>
#include <synch.h>
#include <kern/errno.h>
#include <filetable.h>
#include <vnode.h>
#include <vfs.h>
#include <uio.h>
#include <machine/trapframe.h>
#include <machine/spl.h>
#include <kern/limits.h>

struct semaphore* wait = NULL;
struct semaphore* t = NULL;
struct semaphore* file = NULL;
struct semaphore* forksem = NULL;
struct semaphore* exit = NULL;

extern struct filetable* create_ft();
extern void as_destroy(struct addrspace*);
extern int as_copy(struct addrspace*,struct addrspace**);
extern void md_forkentry(void*,unsigned long);
extern int runprogram(const char*,char**,int);
extern void destroy_ft(struct filetable* ft);
extern struct process* p_table[MAX_PROG+1];

int call_from_fork = 0;
int call_from_execv = 0;
/*
 *  err will store error code
 *  if fail to open a file, return -1 , other syscall will check this
 *  if success ,return fd
 */



int sys_open(const char* filename,int flag,int* err) {

    // validate parameter
    if (filename == NULL) {
       *err = EFAULT;
       return -1;
    }

    int result;
    size_t actual;
    // addr check    
    char kbuf[NAME_MAX];
    if ((result = copyinstr((const_userptr_t)filename, kbuf, NAME_MAX, &actual)) != 0){
        *err = result;
        return -1;
    }

    // find the first available slot
    int i;
    for (i = 3 ;curthread->ft[i] != NULL && i < MAX_FILE; i++);

    // error checking
    if (i == MAX_FILE && curthread->ft[i] != NULL) {
       *err = EMFILE; // full
       return -1;
    }
    // now i is the first available slot

    curthread->ft[i] = create_ft();
    curthread->ft[i]->mode = flag;
    // dup filename
    char* name = NULL;
    name = kstrdup(filename);
    // open file
    result = vfs_open(name,flag,&(curthread->ft[i]->file));

    kfree(name);
    // error checking
    if(result) {
       destroy_ft(curthread->ft[i]);
       curthread->ft[i] = NULL;
       *err = result;
       return -1;
    }

    // return
    return i;
}


int sys_close (int fd, int* err){

    // validate parameter
    if (fd < 3 || fd > MAX_FILE || curthread->ft[fd] == NULL){
       *err = EBADF;
       return -1;
    }

    // close
    if (curthread->ft[fd]->dup > 0) {
       curthread->ft[fd]->dup--;
       return 0;
    }

    vfs_close(curthread->ft[fd]->file);

    destroy_ft(curthread->ft[fd]);
    curthread->ft[fd] = NULL;
    return 0;
}

int sys_read(int fd, void* ubuf, size_t len, int* err){
    // validate parameter
    if (fd < 0 || fd > MAX_FILE || curthread->ft[fd] == NULL) {
       *err = EBADF;
       return -1;
    }

    if (ubuf == NULL){
       *err = EFAULT;
       return -1;
    }

    if (curthread->ft[fd]->mode == O_WRONLY){
      *err = EBADF;
      return -1;
    }

    // error checking completed

    int result;
    void* kbuf = kmalloc(len+1);
    struct uio u;

    // this is kernel level I/O setup
    mk_kuio(&u,kbuf,len,curthread->ft[fd]->offset,UIO_READ);


    // read
    if (file == NULL) file = sem_create("file",1);
    P(file);
    result = VOP_READ(curthread->ft[fd]->file, &u);
    V(file);
    curthread->ft[fd]->offset = u.uio_offset;

    if (result) {
       *err = result;
       kfree(kbuf);
       return -1;
    }

    // error checking & copy memory
    result = copyout(kbuf,ubuf,len-u.uio_resid+1);
    if (result){
       *err = result;
       kfree(kbuf);
       return -1;
    }
    kfree(kbuf);
    return len - u.uio_resid;
}


int sys_write(int fd, const void* ubuf ,size_t nbytes, int* err){
    // valadate parameter
    if (fd < 0 || fd > MAX_FILE || curthread->ft[fd] == NULL) {
       *err = EBADF;
       return -1;
    }
    if (ubuf == NULL) {
       *err = EFAULT;
       return -1;
    }
    
    int result;
    struct uio u;
    void* test = kmalloc(nbytes+1);
    result = copyin((const_userptr_t)ubuf,test,nbytes+1);
    if (result){
       *err = result;
       kfree(test);
       return -1;
    }
    kfree(test);
 
    if (curthread->ft[fd]->mode == O_RDONLY){
       *err = EBADF;
       return -1;
    }

    // error checking completed!

    // this is user-level I/O setup, not kernel-level
    mk_uuio(&u,(void*)ubuf,nbytes,curthread->ft[fd]->offset,UIO_WRITE);

    // write
    if (file == NULL) file = sem_create("file",1);
    P(file);
    result = VOP_WRITE(curthread->ft[fd]->file,&u);
    V(file);
    curthread->ft[fd]->offset = u.uio_offset;

    if (result) {
       *err = result;
       return -1;
    }

    return nbytes - u.uio_resid;
}


pid_t sys_fork(struct trapframe* tf, int* err) {
    if(forksem == NULL) forksem = sem_create("thread",0);
    if(t == NULL) t = sem_create("fork",1);
    P(t);

    int result;
    struct trapframe* newTrap = kmalloc(sizeof (struct trapframe));
    if (newTrap == NULL){
       *err = ENOMEM;
       V(t);
       return -1;
    }
    // copy a new tf
    memcpy(newTrap,tf,sizeof(struct trapframe));

    // addrspace
    struct addrspace *newAddr = NULL;

    // copy as
    result = as_copy(curthread->t_vmspace,&newAddr);
    if (result){
       *err = result;
       V(t);
       return -1;
    }   

   
    struct thread* child = NULL; // return value
    
    call_from_fork = 1;
    result = thread_fork(curthread->t_name,(void*)newTrap,(unsigned long)newAddr,md_forkentry,&child);

    if (result) {
       *err = result;
       V(t);
       return -1;
    }


    P(forksem);
    assert(child != NULL);

    return child->pid;
}

pid_t sys_getpid(void){
    return curthread->pid;
}

pid_t sys_waitpid(pid_t pid, int* status, int options,int* err){
    if (wait == NULL) wait = sem_create("wait",1);
    P(wait);
    // wrong option
    if (options != 0){
       *err = EINVAL;
       V(wait);
       return -1;
    }
    // beyond the region
    if (pid <= 0 || pid > MAX_PROG) {
       *err = EINVAL;
       V(wait);
       return -1;
    }
    // invalid pid
    if (p_table[pid] == NULL){
       *err = EINVAL;
       V(wait);
       return -1;
    }
    if (status == NULL){
       *err = EFAULT;
       V(wait);
       return -1;
    }
    
    // pointer legit?
    int test = 1;
    int result = copyin((const_userptr_t)status, &test, sizeof(int));
    if (result) {
       *err = EFAULT;
       V(wait);
       return -1;
    }

    struct process* parent = p_table[curthread->pid];
    struct process* child = p_table[pid];
    assert(child != NULL);
    assert(parent != NULL);

    // right person
    if (child->ppid != curthread->pid) {
       *status = 0;
       V(wait);
       return 0;
    }
   
    // whether child has been exited?
    if (child->exited == 1){
       // grab exitcode and return
       *status = child->exitcode;
       V(wait);
       return pid;
    }
    // check
    assert(child->exited == 0);

    if (child->exit == NULL) child->exit = cv_create("exit");
    if (child->exitlock == NULL) child->exitlock = lock_create("exitlock");

    V(wait);

    lock_acquire(child->exitlock);
    parent->waiting = 1;
    cv_wait(child->exit,child->exitlock);
    parent->waiting = 0;

    // if code runs here, it means child has been exited!
    assert(child->exited == 1);

    *status = child->exitcode;
    lock_release(child->exitlock);

    // no one will care this child any more
    lock_destroy(child->exitlock);
    cv_destroy(child->exit);
    kfree(p_table[pid]);
    p_table[pid] = NULL;

    return pid;
}



 

void sys__exit(int code){
   if (exit == NULL) exit = sem_create("exit",1);

   P(exit);

   pid_t pid = sys_getpid();
   struct process* p = p_table[pid];
   assert(p != NULL);

   int hasParent;
   if (p->ppid == -1) hasParent = 0;
   else hasParent = 1;

   p->exited = 1;
   p->exitcode = code;
   if(hasParent){
      struct process* parent = p_table[p->ppid];
      assert(parent != NULL);

      if(parent->waiting){
         if(p->exitlock == NULL) p->exitlock = lock_create("exitlock");
         if(p->exit == NULL) p->exit = cv_create("exit");
         lock_acquire(p->exitlock);
         cv_broadcast(p->exit,p->exitlock);
         lock_release(p->exitlock);
      }
   }
   V(exit);

   thread_exit();
}

int sys_execv(const char* prog, char** args,int* err){
    // check prog
    int result;
    size_t actual;
    char buf[PATH_MAX];
    if (prog == NULL){
       *err = EFAULT;
       return -1;
    }
    result = copyinstr((const_userptr_t)prog,buf,PATH_MAX,&actual);
    if (result){
       *err = result;
       return -1;
    }
    // empty string
    if (actual == 1){
       *err = EINVAL;
       return -1;
    }

    // check args
    int k;
    result = copyin((const_userptr_t)args,&k,sizeof(int));
    if (result){
       *err = result;
       return -1;
    }


    int nargs = 0;
    while(args[nargs] != NULL) {
       result = copyin((const_userptr_t)args + nargs * 4, &k, sizeof(int));
       if (result) {
          *err = result;
          return -1;
	}
       nargs++;
    }

    // copy to kernel
    char** argv = kmalloc(sizeof(char*) * nargs);
    int n;
    for (n = 0 ; n < nargs; ++n){
       argv[n] = kmalloc(sizeof(char) * PATH_MAX);
       result = copyinstr((const_userptr_t)args[n],argv[n],PATH_MAX,&actual);
       if (result) {
          *err = result;
          return -1;
       }
    }
    // error checking completed

    // we gonna run a new program
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
    
    // mem will be freed in runprogram.c
    call_from_execv = 1;
    result = runprogram(buf,argv,nargs);
    *err = result;
    return -1;
}
