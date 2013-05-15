#ifndef __VM_TLB_H__
#define __VM_TLB_H__

#include <types.h>
#include <machine/ktypes.h>


int tlb_get_rr_victim(void);
int tlb_getsloy(void);
void tlb_invalidate(int i);
void tlb_flush(void);
//void tlb_replace(vaddr_t va,paddr_t pa);




#endif
