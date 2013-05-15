/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */
#include "opt-A1.h"
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

#if OPT_A1
extern int q_addtail(struct queue *,void *);
extern void * q_remhead(struct queue *);
extern int q_empty(struct queue *);
extern struct queue * q_create(int);
extern void q_destroy(struct queue *);
#endif
////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	#if OPT_A1
        lock->status = 0;    // initially unlocked
        lock->target = NULL; // no one has a lock
	#endif
	return lock;
}

void
lock_destroy(struct lock *lock)
{
        #if OPT_A1
	assert(lock != NULL);

        int spl;
        spl = splhigh();
	assert(thread_hassleepers(lock)==0);
        splx(spl);
        #endif

	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
        #if OPT_A1
        // Validate parameter
        assert (lock != NULL);
        assert (in_interrupt == 0);

	int spl;
        // Disable all the interrupts
        spl = splhigh();
        
        // other threads must be waiting while one thread is in critical section        
        while (lock->status == 1){
           thread_sleep(lock);
        }
        
       
        assert (lock->status == 0)
        // enable the lock
        lock->status = 1;
        
        assert(lock->status==1);
        
        lock->target = curthread;
        // enable interrupts
        splx(spl);
        #else
        (void) lock;
        #endif

}

void
lock_release(struct lock *lock)
{
        #if OPT_A1
	// validate parameter
        assert (lock != NULL);
        
        int spl;
        // disable interrupts
        spl = splhigh();
        
        assert (lock_do_i_hold(lock) == 1); // make sure right lock locks the right thread

        // release the lock
        lock ->status = 0;
        assert (lock->status==0); // check

        lock->target = NULL;
        thread_wakeup(lock);
  
        // enable interrupts
        splx(spl);
        #else
       (void) lock;
        #endif
}

int
lock_do_i_hold(struct lock *lock)
{
        assert (lock != NULL);

	return lock->target == curthread;  
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
        #if OPT_A1
	cv->sleeping_list = (struct queue *)q_create(10);
	// add stuff here as needed
	#endif
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	
	kfree(cv->name);
        #if OPT_A1
        q_destroy(cv->sleeping_list);
        #endif
	kfree(cv);
        
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	#if OPT_A1
	// validate parameter
        assert (cv != NULL);
        assert (lock != NULL);
        // disable interrupts
        int spl = splhigh();

        // release the lock
        assert (lock->status == 1);
       
        lock_release(lock);
        assert (lock->status == 0);

        q_addtail(cv->sleeping_list,curthread);
        // sleep
        thread_sleep(curthread);

        
        
        lock_acquire(lock);
        // enable interrupts
        splx(spl);

        #else
        (void) cv;
        (void) lock;
        #endif
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	#if opt_A1
        // validate parameter

        assert (cv != NULL);
        assert (lock != NULL);
        
        // others

        assert (lock_do_i_hold(lock) == 1);
        // disable interrupts
        int spl = splhigh();
       
        if (q_empty(cv->sleeping_list)) goto done;   // signal must be called after wait!
 
        // pick one thread and wake it up
        thread_wakeup((struct thread*) q_remhead(cv->sleeping_list));
       
        
        // enable interrupts
done:   splx(spl);    
        #else
        (void) cv;
        (void) lock;
        #endif
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
       #if OPT_A1
       // validate parameter

        assert (cv != NULL);
        assert (lock != NULL);

       // others

        assert (lock_do_i_hold(lock) == 1);

       // disable interrupts
       int spl = splhigh();
 
       // test
       if (q_empty(cv->sleeping_list)) goto done;

       // wake up all threads
       while (!q_empty(cv->sleeping_list)) {
         thread_wakeup((struct thread*) q_remhead(cv->sleeping_list));
       }
       
       // enable interrupts
done:  splx(spl);
       #else
       (void) cv;
       (void) lock;
       #endif




}









