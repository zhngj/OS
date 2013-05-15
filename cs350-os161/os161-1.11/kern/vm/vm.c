#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <synch.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <uw-vmstats.h>
#include <page.h>

#define DUMBVM_STACKPAGES 12

extern void coremap_bootstrap(void);


extern struct coremap* map;
extern int num_entries;
extern int reversed_entry;
extern paddr_t start_addr;

struct lock* coremap_lock = NULL;
struct lock* tlb_lock = NULL;
struct lock* page_lock = NULL;
struct lock* swap_lock = NULL;
struct cv* cv_pin =NULL;
struct lock* vm_lock = NULL;

int coremap_ready = 0;

void
vm_bootstrap(void)
{
    /*
     initialize locks
     these locks are supposed to be resident in RAM for ever
    */
    coremap_lock = lock_create("coremap");
    page_lock = lock_create("page");
    swap_lock = lock_create("swap");

    /*
     check
    */
    assert(coremap_lock != NULL);
    assert(page_lock != NULL);
    assert(swap_lock != NULL);

    // initialize coremap
    coremap_bootstrap();
    vmstats_init();

    // coremap done
    // take charge of memory management
    coremap_ready = 1;
  
}
  
paddr_t
getppages(unsigned long npages)
{
        int spl;
        paddr_t addr;

        spl = splhigh();
        // coremap not setup
        if (!coremap_ready){
           addr = ram_stealmem(npages);
           splx(spl);
           return addr;
        }

        // coremap setup already
        assert(coremap_ready == 1);

        if (npages == 1){
           addr =  coremap_alloc_one_page(NULL);
           splx(spl);
           return addr;
        }
        else {
           addr =  coremap_alloc_multi_page(npages);
           splx(spl);
           return addr;
       }
        panic("getppages: unexpected return\n");
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(int npages)
{
        paddr_t pa;
        pa = getppages(npages);
        if (pa==0) return 0;
        return PADDR_TO_KVADDR(pa);

        panic("alloc_kpages: unexpected return\n");
}

void
free_kpages(vaddr_t addr)
{
     paddr_t pa = KVADDR_TO_PADDR(addr);

     if (pa < start_addr) (void)addr; // dont do anything
                                      // we have no access to these memory
     else coremap_free(pa);
}









