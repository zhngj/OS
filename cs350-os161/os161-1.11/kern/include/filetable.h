#ifndef _FILE_TABLE_H_
#define _FILE_TABLE_H_

#include <types.h>
#include <vnode.h>
#include <thread.h>
#include <curthread.h>
struct filetable {
    off_t offset;
    struct vnode* file;
    int mode;
    int dup;
};


struct filetable* crate_ft();
void destroy_ft(struct filetable* table);
int conSetup(struct thread*);
#endif
