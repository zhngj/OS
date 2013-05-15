#include <types.h>
#include <machine/ktypes.h>
#include <lib.h>
#include <page.h>
#include <coremap.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <swapfile.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <uw-vmstats.h>
#include "opt-A3.h"
#if OPT_A3

extern struct coremap* map;
extern int coremap_pages;
extern int num_entries;
extern struct lock* page_lock;

// no need to protect
struct page_dir*
make_page_dir(vaddr_t va,size_t size,int read,int write,int exec)
{
   struct page_dir* ret = NULL;

   ret = (struct page_dir*)kmalloc(sizeof(struct page_dir));
   if (ret == NULL) return NULL;

   ret->vbase = va;
   ret->vtop = 0x0;

   /* permission */
   ret->read = read;
   ret->write = write;
   ret->exec = exec;

   // now each page
   ret->pte = kmalloc(sizeof(struct page*) * size);
   return ret;
}

// no need to protect
struct page*
make_page()
{
   struct page* ret;
   ret = (struct page*)kmalloc(sizeof(struct page));
   if (ret == NULL) return NULL;
   
   ret->pa = INVALID_PADDR;
   ret->va = 0x0;
   ret->swap_loc = INVALID_SWAP;
   ret->valid = -1;

   return ret;
}


// 
//
int
page_zerofill(struct page** ret,vaddr_t base,int index)
{
  
   assert(lock_do_i_hold(page_lock) == 0);

   struct page* p = NULL;
   p = make_page();
   assert(p != NULL);
   assert(p->valid == -1);
   assert(p->va == 0x0);
   assert(p->pa == INVALID_PADDR);

   p->swap_loc = INVALID_SWAP; // swap_loc
   
   paddr_t pa = coremap_alloc_user(p);
   if (pa == INVALID_PADDR) return ENOMEM;

   p->pa = pa; // pa
   p->valid = 1; // valid
   p->va = base + PAGE_SIZE*index;
   
   // zero pa;
   coremap_zero_page(pa);

   *ret = p;
   return 0;
}


int
page_fault(struct addrspace* as, struct page* p, int faulttype, vaddr_t fa,int writeable)
{
   assert(p != NULL);

   paddr_t pfn = p->pa & TLBLO_PPAGE;
   u_int32_t elo,ehi;
   int map_index;
   int tlb_index;
   int victim = 0;
   // Note:
   // here we must determine whether we need read from elf or swapfile
   if (pfn == INVALID_PADDR){ // swapfile
      assert(p->valid == 0);
      assert(p->swap_loc != INVALID_SWAP);
      paddr_t pa = coremap_alloc_user(p);

      map_index = PADDR_TO_COREMAP(pa);
      p->valid = 1;
      p->pa = pa;
      p->va = fa & PAGE_FRAME;

      assert(map[map_index].p == p);
      assert(pa != INVALID_PADDR);
      assert(p->swap_loc != INVALID_SWAP);

      swap_in(pa,p->swap_loc); // read from swap file

      swap_free(p->swap_loc); // read complete, release the space
      p->swap_loc = INVALID_SWAP;

      assert(p->pa != INVALID_PADDR);     

      // update TLB
      assert((pa & PAGE_FRAME) == pa);

      elo = pa | TLBLO_VALID;
      ehi = fa & TLBHI_VPAGE;
      if ((faulttype == VM_FAULT_WRITE && writeable) || writeable){
         elo = elo | TLBLO_DIRTY;
      }
      // probe TLB slot
      // if we get a slot,fine
      // if we get -1, we must do a eviction
      tlb_index = TLB_Probe(fa,0);
      if (tlb_index == -1) {
         tlb_index = tlb_getslot();
         assert(tlb_index != -1);
         assert(tlb_index < NUM_TLB);
         TLB_Write(ehi,elo,tlb_index);
      }
     else {
         vmstats_inc(1);
         TLB_Write(ehi,elo,tlb_index);
      }
   }
   else{ // elf
         // just update tlb, because we have already allocated a physical frame before
      elo = p->pa | TLBLO_VALID;
      ehi = fa & TLBHI_VPAGE;
      if ((faulttype == VM_FAULT_WRITE && writeable) || writeable){
         elo |= TLBLO_DIRTY;
      }
      tlb_index = TLB_Probe(fa,0);
      if (tlb_index == -1){
         tlb_index = tlb_getslot();
         assert(tlb_index != -1);
         TLB_Write(ehi,elo,tlb_index);
      }
      else {
         assert(tlb_index >= 0);
         vmstats_inc(1);
         TLB_Write(ehi,elo,tlb_index);
      }
      map_index = PADDR_TO_COREMAP(p->pa);
      assert(map_index >= 0);
      assert(map_index < num_entries);
      assert(p->valid == 1);
      assert(map[map_index].p == p);
   }
   return 0;
}


// evict a page from RAM
// protected

void
page_evict(struct page* p)
{
   assert(p != NULL);
   assert(lock_do_i_hold(page_lock) == 0);

   lock_acquire(page_lock);

   p->swap_loc = swap_alloc(); // alloc a swap space

   assert(p->swap_loc != INVALID_SWAP);

   paddr_t pa = p->pa & PAGE_FRAME;
   
   int valid = p->valid;
   if (valid == 1){

      swap_out(pa,p->swap_loc);
      p->valid = 0; // indicate not in RAM

   }else{panic("evict a invalid page\n");}

   assert(p->valid == 0);
   p->pa = INVALID_PADDR;

   lock_release(page_lock);
}
#endif
