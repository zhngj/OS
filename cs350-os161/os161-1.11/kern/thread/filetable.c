#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <filetable.h>
#include <curthread.h>
#include <thread.h>
#include <vfs.h>
#include <synch.h>
struct filetable* create_ft(){

   struct filetable* ft = kmalloc(sizeof(struct filetable));
   if (ft == NULL) return NULL;

   ft->offset = 0;
   ft->mode = -1;
   ft->dup = 0;
   ft->file = NULL;
   return ft;
}

inline void destroy_ft(struct filetable* ft){
     assert (ft != NULL);
     kfree(ft);
}


int conSetup(struct thread* t){
        if (t->ft[0] != NULL || t->ft[1] != NULL || t->ft[2] != NULL) return 0;
        int result;
        char* console = kstrdup("con:");
        if (console == NULL) return ENOMEM;

        /* connect to stdin */
        t->ft[0] = create_ft();
        if (t->ft[0] == NULL) goto fail;

        result = vfs_open(console,O_RDONLY,&(t->ft[0]->file));
        if (result) goto fail;

        t->ft[0]->mode = O_RDONLY;
        assert(t->ft[0]->file != NULL);
        kfree(console);

        /* connect to stdout */
        console = kstrdup("con:");
        if(console == NULL){
            return ENOMEM;
        }

        t->ft[1] = create_ft();
        if(t->ft[1] == NULL) goto fail;

        result = vfs_open(console,O_WRONLY,&(t->ft[1]->file));
        if (result) goto fail;

        t->ft[1]->mode = O_WRONLY;
        assert(t->ft[1]->file != NULL);

        kfree(console);

        /* connect to stderr */
        console = kstrdup("con:");
        if(console == NULL){
           return ENOMEM;
        }

        t->ft[2] = create_ft();
        if (t->ft[2] == NULL) goto fail;

        result = vfs_open(console,O_WRONLY,&(t->ft[2]->file));
        if (result) goto fail;

        t->ft[2]->mode = O_WRONLY;
        assert(t->ft[2]->file != NULL);

        kfree(console);

        return 0;

fail:   kfree(console);
        return result;
}

struct filetable* copy_ft(struct filetable* old){
   struct filetable* new = create_ft();
   if (new == NULL) return NULL;

   new->file = old->file;
   new->offset = old->offset;
   new->mode = old->mode;
   new->dup = old->dup + 1;
   return new;
}






















