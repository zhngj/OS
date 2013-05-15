#ifndef __PAGE_H__
#define __PAGE_H__

#include "opt-A3.h"
#include <types.h>
#include <machine/ktypes.h>
#include <addrspace.h>
#if OPT_A3


/* second level */
struct page {
  volatile paddr_t pa;
  volatile vaddr_t va;
  off_t swap_loc;
  int valid;
};  

/* first level */
struct page_dir {
  int read;
  int write;
  int exec;
  int npages;
  vaddr_t vbase;
  vaddr_t vtop;
  struct page** pte; /* an array of pages */
};


struct page_dir* make_page_dir(vaddr_t va, size_t size,int r,int w, int e);
struct page* make_page();
int page_fault(struct addrspace* as, struct page* p, int faulttype, vaddr_t fa,int w);
int page_zerofill(struct page** ret,vaddr_t va, int index);
void page_evict(struct page* p);


#endif

#endif /* page.h */
