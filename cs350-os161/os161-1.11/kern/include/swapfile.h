#ifndef __SWAP_FILE_H__
#define __SWAP_FILE_H__

#include <types.h>
#include <machine/ktypes.h>
#include "opt-A3.h"

#if OPT_A3

#define INVALID_SWAP (0)

void swap_bootstrap(void);
void swap_shutdown(void);
off_t swap_alloc(void);
void swap_free(off_t loc);
void swap_in(paddr_t pa,off_t loc);
void swap_out(paddr_t pa, off_t loc);

#endif

#endif /* swapfile.h */
