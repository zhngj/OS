#include <types.h>
#include <machine/ktypes.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <bitmap.h>
#include <synch.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <vm.h>
#include <coremap.h>
#include <swapfile.h>
#include <uw-vmstats.h>
#include "opt-A3.h"

#if OPT_A3

struct vnode* swap_file = NULL;
struct bitmap* swap_map = NULL;
static char filename[] = "SWAPFILE";

unsigned long swap_size = 9 * 1024 * 1024;
unsigned long swap_total_pages;
unsigned long swap_free_pages;

extern struct lock* swap_lock;

void
swap_bootstrap(void)
{

    char* file = NULL;
    file = kstrdup(filename);
    assert(file != NULL);

    int result = vfs_open(file, O_RDWR|O_CREAT|O_TRUNC, &swap_file);
    if (result){
       panic("swap: error in opening swap file\n");
    }
    
    swap_total_pages = swap_size / PAGE_SIZE;
    swap_free_pages = swap_total_pages;
   
    swap_map = bitmap_create(swap_total_pages);
    if (swap_map == NULL){
       panic("swap: not enough memory creating swapmap\n");
    }

    bitmap_mark(swap_map,0);
    swap_free_pages--;
    kfree(file);
}

void
swap_shutdown(void)
{
    assert(swap_lock != NULL);
    assert(swap_map != NULL);
    assert(swap_file != NULL);

    lock_destroy(swap_lock);
    bitmap_destroy(swap_map);
    vfs_close(swap_file);
}

off_t
swap_alloc(void)
{
    assert(swap_map != NULL);
    assert(lock_do_i_hold(swap_lock) == 0);

    lock_acquire(swap_lock);
    
    assert(swap_free_pages > 0);
    assert(swap_free_pages <= swap_total_pages);

    int index;
    int rc = bitmap_alloc(swap_map,&index);
    if (rc){
       panic("bitmap: Out of swap space\n");
    }

    /* update */
    swap_free_pages--;
    assert(index > 0);
    lock_release(swap_lock);
    
    return index * PAGE_SIZE; /* this is location of a page in swap file */
}

void
swap_free(off_t loc)
{
    assert(swap_lock != NULL);
    assert(swap_map != NULL);
    /* make sure this location is legit */
    assert(loc % PAGE_SIZE == 0);

    lock_acquire(swap_lock);
    
    int index = loc / PAGE_SIZE;
    bitmap_unmark(swap_map,index); /* mark it as unused */

    /* update */
    swap_free_pages++;
    lock_release(swap_lock);
}

void
swap_in(paddr_t pa, off_t loc)
{
    /* error checking */
    assert(loc % PAGE_SIZE == 0)       // page-aligned
    assert(bitmap_isset(swap_map,loc / PAGE_SIZE));
    assert(lock_do_i_hold(swap_lock) == 0);

    
    lock_acquire(swap_lock);
    struct uio u;
    vaddr_t va;
    
    va = PADDR_TO_KVADDR(pa);
    mk_kuio(&u,(char*)va,PAGE_SIZE,loc,UIO_READ);
    vmstats_inc(8);
    int result = VOP_READ(swap_file,&u);
    if (result){
       panic("swap: error when trying to swap in\n");
    }
    assert(u.uio_resid == 0);
    lock_release(swap_lock);

}

void
swap_out(paddr_t pa, off_t loc)
{
    /* error checking */
    assert(loc % PAGE_SIZE == 0)       // page-aligned
    assert(bitmap_isset(swap_map,loc / PAGE_SIZE));
    assert(lock_do_i_hold(swap_lock) == 0);

    lock_acquire(swap_lock);
    
    struct uio u;
    vaddr_t va;
        
    va = PADDR_TO_KVADDR(pa);
    mk_kuio(&u,(char*)va,PAGE_SIZE,loc,UIO_WRITE);
    vmstats_inc(9);
    int result = VOP_WRITE(swap_file,&u);
    if (result){
       panic("swap: error when trying to swap out\n");     
    }
    lock_release(swap_lock);
}

#endif
