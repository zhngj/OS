#include <types.h>
#include <machine/ktypes.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <addrspace.h>
#include <lib.h>
#include <uw-vmstats.h>
#include <vm_tlb.h>
#include "opt-A3.h"


#if OPT_A3


int
tlb_get_rr_victim(void)
{
    vmstats_inc(2);
    int victim;
    static unsigned int next_victim = 0;
   
    victim = next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;
}


void
tlb_invalidate(int slot)
{
    int spl = splhigh();
    u_int32_t elo,ehi;
    TLB_Read(&ehi,&elo,slot);
    if (elo & TLBLO_VALID) TLB_Write(TLBHI_INVALID(slot),TLBLO_INVALID(),slot);
    splx(spl);
}


void
tlb_flush(void)
{
    vmstats_inc(3);
    int i;
    for (i = 0 ; i < NUM_TLB; ++i){
       tlb_invalidate(i);
    }
}


/*
 we must firstly probe, if return -1 then we can use this function
*/
int
tlb_getslot(void)
{  
   int i;
   u_int32_t ehi,elo;

   for(i = 0 ; i < NUM_TLB; ++i){
      TLB_Read(&ehi,&elo,i);
      if (elo & TLBLO_VALID) continue;
      vmstats_inc(1);
      return i;
   }
   int victim = tlb_get_rr_victim();
   tlb_invalidate(victim);
   return victim;
}


void
tlb_replace(vaddr_t fa,paddr_t pa)
{
   int i,elo,ehi;
        
   /* make sure it's page-aligned */
   assert((pa & PAGE_FRAME)==pa);
       
   for (i = 0; i < NUM_TLB; i++) {
       TLB_Read(&ehi, &elo, i);
       if (elo & TLBLO_VALID) {
          continue;
       }
       ehi = fa;
       elo = pa | TLBLO_DIRTY | TLBLO_VALID;
       TLB_Write(ehi, elo, i);
       //vmstats_inc(VMSTAT_TLB_FAULT_FREE);
       return;
   }

   assert(i == NUM_TLB);
   // full, need eviction
   ehi = fa;
   elo = pa | TLBLO_DIRTY | TLBLO_VALID;
   TLB_Write(ehi,elo,tlb_get_rr_victim());
   //vmstats_inc(VMSTAT_TLB_FAULT_REPLACE);
}



#endif































