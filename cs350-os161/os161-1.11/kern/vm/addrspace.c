#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <elf.h>
#include <coremap.h>
#include <syscall.h>
#include <swapfile.h>
#include <coremap.h>
#include <vm_tlb.h>
#include <vnode.h>
#include <vm.h>
#include <uw-vmstats.h>
#include <machine/spl.h>
#include <machine/tlb.h>

extern int num_entries;
extern struct coremap* map;

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */
#define DUMBVM_STACKPAGES 12
#define stackbase (USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE)

extern struct lock* vm_lock;

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	int i,result;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;
	spl = splhigh();

        // get vpn
        faultaddress &= TLBHI_VPAGE;

        // check address
        assert(faultaddress < MIPS_KSEG0);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
                splx(spl);
                sys__exit(-1);
                panic("return from sys_exit()\n");
	    case VM_FAULT_READ:
                 
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}
	as = curthread->t_vmspace;
	if (as == NULL){
           splx(spl);
           panic("empty as\n");
           return EFAULT;
        }
        vmstats_inc(0);

        vaddr_t base,top;
        struct page_dir* target = NULL;
        struct page* ret = NULL;

        // size of segments
        int size = 3;
        // page walk
        struct page_dir* dir;
	for (i = 0; i < size; i++){
           dir = as->pt[i];
           assert(dir != NULL);

           base = dir->vbase;
           top = dir->vtop;

           if (faultaddress >= base && faultaddress < top){
              target = dir;
              assert(target != NULL);
              break;
           }
        }

        assert(dir != NULL);
        assert(target != NULL);

        int writeable = dir->write;
        int index = (faultaddress - base) / PAGE_SIZE;

        ret = target->pte[index];
        // whether we first access?
        if (ret == NULL){
           vmstats_inc(5);
           vmstats_inc(6);
           vmstats_inc(7);
           result = page_zerofill(&ret,dir->vbase,index);
           if (result){
              splx(spl);
              panic("vm_fault: error in creating page\n");
              return result;
           }
           assert(ret != NULL);
           assert(ret->valid == 1);
           target->pte[index] = ret;
           assert(target->pte[index] != NULL);
        }
        else {vmstats_inc(4);}
        //ret->va = faultaddress;
      
        //assert(target->pte[index]->va == faultaddress);
  
        // deal with page_fault
        result = page_fault(as,ret,faulttype,faultaddress,writeable);
        assert(result == 0);
        splx(spl);
        return 0;
}

struct addrspace *
as_create(void)
{
        struct addrspace *as = kmalloc(sizeof(struct addrspace));
        if (as==NULL) {
                return NULL;
        }
        as->pt = kmalloc(sizeof(struct page_dir*) * 3);
        int i;
        for(i = 0 ; i < 3 ; ++i){
           as->pt[i] = NULL;
        }
        return as;

}

void
as_destroy(struct addrspace *as)
{
        int i;
        for(i = 0 ; i < 3; ++i){
           int size = as->pt[i]->npages;

           int j;
           for(j = 0 ; j < size; ++j){
              struct page* p = as->pt[i]->pte[j];
              if (p != NULL){
                 kfree(p);
              }
           }
           kfree(as->pt[i]);
        }
	kfree(as);
        for(i = 0; i < num_entries; ++i) {
           if (map[i].who == KERNEL) continue;
           map[i].status = FREE;
           map[i].who = UNKNOWN;
           map[i].last = 0;
           map[i].p = NULL;
        }
}

void
as_activate(struct addrspace *as)
{
        int i, spl;

        (void)as;
        spl = splhigh();

        tlb_flush();

        splx(spl);

}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as,vaddr_t vaddr, size_t sz,
		 int readable, int writeable,int exec)
{
        size_t npages;

        /* Align the region. First, the base... */
        sz += vaddr & ~(vaddr_t)PAGE_FRAME;
        vaddr &= PAGE_FRAME;

        /* ...and now the length. */
        sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

        npages = sz / PAGE_SIZE;
        
        int r = 0,w = 0,x = 0;
        if(readable) r = 1;
        if(writeable) w = 1;
        if(exec) x = 1;

        /* build two-level page table */
        struct page_dir* dir;
        dir = make_page_dir(vaddr,npages,r,w,x);

        if (dir == NULL) return ENOMEM;

        assert(dir->vbase == vaddr);
        assert(dir->pte != NULL);
        
        dir->vtop = dir->vbase + PAGE_SIZE * npages;
        dir->npages = npages;
        int i;
        for(i = 0; i < npages; ++i) dir->pte[i] = NULL;

        assert((dir->vtop - dir->vbase) / PAGE_SIZE == npages);
     //   kprintf("base: %x, top: %x\n",dir->vbase,dir->vtop); 
        if (vaddr == 0x00400000) as->pt[0] = dir;
        if (vaddr == 0x10000000) as->pt[1] = dir;
        if (vaddr == stackbase)  as->pt[2] = dir;
        assert(vaddr == 0x00400000 || vaddr == 0x10000000 || vaddr == stackbase);
        
        return 0;

}

int
as_prepare_load(struct addrspace *as)
{      
       as->pt[0]->write = 1; /* code segment */
       return 0;

}

int
as_complete_load(struct addrspace *as)
{
	as->pt[0]->write = 0;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
        assert(as != NULL);

        int result = as_define_region(as,stackbase,DUMBVM_STACKPAGES * PAGE_SIZE,1,1,0);
        if (result) return result;

        *stackptr = USERSTACK;
        return 0;

}


int
as_copy(struct addrspace *old, struct addrspace **ret)
{
        
        struct addrspace *new;

        new = as_create();
        if (new==NULL) {
                return ENOMEM;
        }

        new->pt = kmalloc(sizeof(struct page_dir*) * 3);
        int i;
        for(i = 0 ; i < 3 ; ++i){
           new->pt[i] = old->pt[i];
        }    
        return 0;
}
