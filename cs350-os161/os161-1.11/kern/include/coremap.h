#ifndef __COREMAP_H__
#define __COREMAP_H__

#include <types.h>
#include <machine/ktypes.h>
#include <addrspace.h>
#include "opt-A3.h"

#if OPT_A3

#define INVALID_PADDR ((paddr_t)0)
#define COREMAP_TO_PADDR(map)   (((paddr_t)PAGE_SIZE)*((map)+coremap_pages))
#define PADDR_TO_COREMAP(page)  (((page)/PAGE_SIZE)-coremap_pages)

struct page;

typedef enum {
   USED,
   FREE,
} state;

typedef enum {
   UNKNOWN,
   USER,
   KERNEL
} people;

struct coremap {
    struct page* p;
    state status;
    people who;
    int last;
};

/* start up */
void coremap_bootstrap(void);

/* allocation and deallocation */
paddr_t coremap_alloc_page(struct page* p);
paddr_t coremap_alloc_multi_page(unsigned long npages);
void coremap_free(paddr_t pa);
paddr_t coremap_alloc_user(struct page* p);

/* others */
void coremap_zero_page(paddr_t pa);
void mark_pages_allocated(int base, int npages,int iskern);
int page_replace(void);
int do_page_replace(void);
void evict_ram(int loc);
#endif

#endif /* coremap.h */
