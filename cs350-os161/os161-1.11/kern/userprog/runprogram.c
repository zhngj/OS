/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include "opt-A2.h"
/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
extern int call_from_execv;
int
runprogram(char *progname,char** args, int nargs)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}
      
        #if OPT_A2
        stackptr -= (sizeof (char*) * (nargs + 1)); // dont forget NULL
        userptr_t *u_args = (userptr_t*)stackptr;
        int i;
        for (i = 0 ; i < nargs ; ++i  ){
           int len = strlen(args[i]) + 1;
           stackptr -= len;
           u_args[i] = (userptr_t)stackptr;
           copyout(args[i], u_args[i], sizeof(char) * len );
                 
        }
        u_args[nargs] = NULL;

        stackptr -= stackptr % 8; //align 
        /* dont forget free memory! */
        if (call_from_execv){
           for(i = 0; i < nargs ;++i) kfree(args[i]);
           kfree(args);
           call_from_execv = 0;
        }
        assert(call_from_execv == 0);

	/* Warp to user mode. */
	md_usermode(nargs /*argc*/, (userptr_t)u_args /*userspace addr of argv*/,
		    stackptr, entrypoint);
	#endif
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}


