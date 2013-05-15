#include <types.h>
#include <machine/ktypes.h>
#include <machine/spl.h>
#include <lib.h>
#include <curthread.h>
#include <thread.h>
#include <synch.h>
#include <machine/vm.h>
#include <addrspace.h>
#include <coremap.h>
#include <uw-vmstats.h>
#include "opt-A3.h"

/* global coremap */
struct coremap* map = NULL;


int num_entries;   // # of pages can be used
int coremap_pages; // # of pages used for coremap
paddr_t start_addr;
int rep_index = 0;
extern struct lock* coremap_lock;

void
coremap_bootstrap(void)
{
    paddr_t first,last;
    int pages;
    size_t request_coremap_size;

    ram_getsize(&first,&last);

    assert ((first & PAGE_FRAME) == first);
    assert ((last  & PAGE_FRAME) == last);
    /*
     cannot call kmalloc() now
     because ram_stealmem() won't work
     we must manually allocate memory for coremap
    */
    pages = (last - first) / PAGE_SIZE; // get total pages
    request_coremap_size = pages * sizeof(struct coremap); // get total size of coremap
    request_coremap_size = ROUNDUP(request_coremap_size,PAGE_SIZE); // page-aligned
    
    // check
    assert((request_coremap_size & PAGE_FRAME) == request_coremap_size);

    // now allocate memory manually for coremap
    start_addr = first;
    map = (struct coremap*)PADDR_TO_KVADDR(first);
    assert(map != NULL);

    // resize memory
    first += request_coremap_size;
    
    // make sure we dont run out memory
    if (first >= last) panic("coremap: sys memory size is two small\n");

    // calculate how many entries we need
    coremap_pages = first / PAGE_SIZE; // page for coremap
    num_entries = (last - first) / PAGE_SIZE;
  
    //assert (num_entries + coremap_pages == pages);
    
    /*
      now we initialzie every coremap entry
    */
    int j;
    for (j = 0; j < num_entries ;++j){
       map[j].status = FREE;
       map[j].who = UNKNOWN;
       map[j].p = NULL;
       map[j].last = 0; /* whether the block reached the end */
    }
}

int
page_replace(void)
{
    int i;
    for (i=rep_index; i<num_entries; i++){
        if (map[i].who != KERNEL){
            rep_index = i + 1;
            return i;
        }
    }
    for (i = 0; i < rep_index; i++){
        if (map[i].who != KERNEL){
            rep_index = i + 1;
            return i;
        }
    }
    return -2;
}

int
do_page_replace(void)
{
	int where;

	where = page_replace();
        if (where == -2) return -2;

	assert(map[where].who == USER);
        evict_ram(where);
	
	return where;
}


// protected
void
evict_ram(int loc)
{
   assert(lock_do_i_hold(coremap_lock));
   struct page* p = NULL;
   assert(map[loc].who != KERNEL);
   assert(map[loc].status == USED);

   p = (struct page*) map[loc].p;
   assert(COREMAP_TO_PADDR(loc) == (p->pa & PAGE_FRAME));
   assert(p != NULL);
   assert((p->pa & PAGE_FRAME) != INVALID_PADDR);
   if (p->valid != 1)panic("%d\n",p->valid);   

   page_evict(p);
   int spl = splhigh();
   // need to invalidate TLB
//   assert(p->va != 0x0);

  tlb_flush();
   /* 
   int tlb_index = TLB_Probe(p->va,0);
   if (tlb_index >= 0){
      tlb_invalidate(tlb_index);
   }
   else { assert(tlb_index == -1);} // not in TLB

   assert(COREMAP_TO_PADDR(loc) == (p->pa & PAGE_FRAME));
   */
   splx(spl);


   // at this time the evicted page has been in swap file
   assert(p->valid == 0);
   assert(p->pa == INVALID_PADDR);
//   assert(p->va == 0x0);

  // clean the coremap slot
   map[loc].status = FREE;
   map[loc].who = UNKNOWN;
   map[loc].p = NULL;


   return;
}




paddr_t
coremap_alloc_one_page(struct page* p)
{
    int iskern = (p == NULL);

    assert(lock_do_i_hold(coremap_lock) == 0);
   
    lock_acquire(coremap_lock);

    int i,person = -1;
    for(i = num_entries-1; i >=0; i--){
       assert(map[i].who == KERNEL || map[i].who == USER || map[i].who == UNKNOWN);

       if (map[i].who == KERNEL) continue;

       if (map[i].status == FREE){
          person = i;
          break;
       }
    }
    if (person == -1){ // do eviction
       person = do_page_replace();
       if (person == -2) {
          lock_release(coremap_lock);
          return INVALID_PADDR;
       }
    }
    else{
       assert(person != -1);
       assert(map[person].status == FREE);
    }

    assert(person != -1);
    assert(map[person].status == FREE);
    assert(map[person].who == UNKNOWN);
    assert(map[person].p == NULL);

    mark_pages_allocated(person,1,iskern);
    if (iskern) {assert(p == NULL);}
    else{
       assert(p != NULL);
    }
    map[person].p = p;

    lock_release(coremap_lock);
    return COREMAP_TO_PADDR(person);
    
}


// allocate from down to top

paddr_t
coremap_alloc_multi_page(unsigned long npages)
{    
     lock_acquire(coremap_lock);
     
     int user,count,base; // pages need to be evicted
     int i;
     count = 0;
     user = 0;
     base = -1;

     for(i = 0 ; i < num_entries; ++i){
        // alloc
        if (map[i].status == USED) {
           // recalculate
           count = 0;
           continue;
        }
        if (map[i].status == FREE){
           count++;
        }
       if (count == npages) break;
     }
    //  test whether we probe a empty space
     if (count == npages){
        base = i+ 1 - npages;
        assert(map[base].status == FREE);
        mark_pages_allocated(base,npages,1/*kernel*/);
        lock_release(coremap_lock);
        return COREMAP_TO_PADDR(base);
     }
     else{
       assert(i == num_entries);
     }

     count = 0;
     user = 0;
     for(i = 0 ; i < num_entries; ++i){
        if (map[i].status == FREE){
           count++;
        }
        if (map[i].status == USED){
          // if user use this page, it may be evicted
          if (map[i].who == USER){
             user++;
          }
          else{
          // reset everything
             assert(map[i].who == KERNEL);
             user = 0;
             count = 0;
         }
       }
       if (count + user == npages) break;
     }
     // test whether we are able to find a space
     if (count + user != npages){
     // fail!
        lock_release(coremap_lock);
        return INVALID_PADDR;
     }
     // some user page need to be evicted
     assert(count+user == npages);
     base = i + 1 - npages;

     for(i = base; i < base+npages; ++i){
        if (map[i].status == FREE) continue;
        if (map[i].who == USER) evict_ram(i);

        // we should now have a clean page
        assert(map[i].status == FREE);
     }
     mark_pages_allocated(base,npages,1/*kernel*/);

     lock_release(coremap_lock);
     return COREMAP_TO_PADDR(base);
}   

void
coremap_free(paddr_t pa)
{
    lock_acquire(coremap_lock);

    assert (pa != INVALID_PADDR);
    int index = PADDR_TO_COREMAP(pa);

    assert(index < num_entries);
    assert(index >= 0);

    int i;
    for(i = index; i < num_entries; ++i){
       assert(map[i].status == USED);
       
       map[i].status = FREE;
       map[i].who = UNKNOWN;
       map[i].p = NULL;
       if (map[i].last) {
          break;
       }
       map[i].last = 0;
   }

   lock_release(coremap_lock);
 
}

paddr_t
coremap_alloc_user(struct page* p)
{  
   assert(p != NULL);
   return coremap_alloc_one_page(p);
}

// zero-out a page
void
coremap_zero_page(paddr_t pa)
{
   assert(pa != INVALID_PADDR);

   vaddr_t va = PADDR_TO_KVADDR(pa);
   bzero((char*)va,PAGE_SIZE);

}




// mark each page as USED, and set appropriate bit
void
mark_pages_allocated(int base, int npages, int iskern)
{
    int i;
    for(i = base ; i < base+npages; ++i){
       assert(map[i].who == UNKNOWN || map[i].who == USER);
       assert(map[i].who != KERNEL);
       assert(map[i].status == FREE);

       map[i].status = USED;

       if(iskern){
          map[i].who = KERNEL;
       }
       else{
          map[i].who = USER;
       }
   }
   map[base+npages-1].last = 1;
}


