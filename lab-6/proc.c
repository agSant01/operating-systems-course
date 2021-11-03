/* This file contains essentially all of the process and message handling.
 * Together with "mpx.s" it forms the lowest layer of the MINIX kernel.
 * There is one entry point from the outside:
 *
 *   sys_call: 	      a system call, i.e., the kernel is trapped with an INT
 *
 * Changes:
 *   Aug 19, 2005     rewrote scheduling code  (Jorrit N. Herder)
 *   Jul 25, 2005     rewrote system call handling  (Jorrit N. Herder)
 *   May 26, 2005     rewrote message passing functions  (Jorrit N. Herder)
 *   May 24, 2005     new notification system call  (Jorrit N. Herder)
 *   Oct 28, 2004     nonblocking send and receive calls  (Jorrit N. Herder)
 *
 * The code here is critical to make everything work and is important for the
 * overall performance of the system. A large fraction of the code deals with
 * list manipulation. To make this both easy to understand and fast to execute 
 * pointer pointers are used throughout the code. Pointer pointers prevent
 * exceptions for the head or tail of a linked list. 
 *
 *  node_t *queue, *new_node;	// assume these as global variables
 *  node_t **xpp = &queue; 	// get pointer pointer to head of queue 
 *  while (*xpp != NULL) 	// find last pointer of the linked list
 *      xpp = &(*xpp)->next;	// get pointer to next pointer 
 *  *xpp = new_node;		// now replace the end (the NULL pointer) 
 *  new_node->next = NULL;	// and mark the new end of the list
 * 
 * For example, when adding a new node to the end of the list, one normally 
 * makes an exception for an empty list and looks up the end of the list for 
 * nonempty lists. As shown above, this is not required with pointer pointers.
 */

#include <minix/com.h>
#include <minix/ipcconst.h>
#include <stddef.h>
#include <signal.h>
#include <assert.h>

#include "kernel/kernel.h"
#include "vm.h"
#include "clock.h"
#include "spinlock.h"
#include "arch_proto.h"

#include <minix/syslib.h>

/*===========================================================================*
 *				Other Unmodified Code					     * 
 *===========================================================================*/

/*===========================================================================*
 *				enqueue					     * 
 *===========================================================================*/
void enqueue(
	register struct proc *rp /* this process is now runnable */
)
{
	/* Add 'rp' to one of the queues of runnable processes.  This function is 
 * responsible for inserting a process into one of the scheduling queues. 
 * The mechanism is implemented here.   The actual scheduling policy is
 * defined in sched() and pick_proc().
 *
 * This function can be used x-cpu as it always uses the queues of the cpu the
 * process is assigned to.
 */
	int q = rp->p_priority; /* scheduling queue to use */
	struct proc **rdy_head, **rdy_tail;

	assert(proc_is_runnable(rp));

	assert(q >= 0);

	rdy_head = get_cpu_var(rp->p_cpu, run_q_head);
	rdy_tail = get_cpu_var(rp->p_cpu, run_q_tail);

	/*********************************************************
	*                Start of EDF Modification               *
	*********************************************************/
	/* Now add the process to the queue. */
	if (!rdy_head[q])
	{
		/* add to empty queue.
		* The deadline does not matter, because the queue is empty.
		*/
		rdy_head[q] = rdy_tail[q] = rp; /* create a new queue */
		rp->p_nextready = NULL;			/* mark new end */
	}
	else if (rp->deadline == 0)
	{
		/* we do not care of the order of this process.
		* add to tail of queue 
		*/
		rdy_tail[q]->p_nextready = rp; /* chain tail of queue */
		rdy_tail[q] = rp;			   /* set new queue tail */
		rp->p_nextready = NULL;		   /* mark new end */
	}
	else
	{
		/* Deadline is > 0. 
		* Iterate over exiting queue to insert the rp in the position it has to be.
		*/
		struct proc *prev = NULL;		 /* var to store the prev reference */
		struct proc *temp = rdy_head[q]; /* first process in queue of priority 'q' */

		/* Iterate over queue of processes.
		* While not the end of list and while the deadline of the process set to run is greater
		* than the deadline of the temp process. */
		while (temp != NULL && temp->deadline != 0 && rp->deadline > temp->deadline)
		{
			prev = temp;
			temp = temp->p_nextready;
		}
		/* Here "temp" is either NULL or a proc:
		*	- NULL: Means that no other process had a grater deadline, set this rp to the end
		*		of the queue with priority 'q'
		*	- proc instance: rp.deadline is smaller/equal to the temp.deadline. 
		*		Set rp before temp in queue.
		*/

		if (prev == NULL)
		{								   /* rp should be the head of the queue */
			rp->p_nextready = rdy_head[q]; /* set next of rp the current head */
			rdy_head[q] = rp;			   /* set rp to be the head of the queue */
		}
		else if (temp == NULL)
		{								   /* reached the end of the queue. */
										   /* set the tail of the queue to this process */
			rdy_tail[q]->p_nextready = rp; /* chain tail of queue */
			rdy_tail[q] = rp;			   /* set new queue tail */
			rp->p_nextready = NULL;		   /* mark new end */
		}
		else
		{ /* Not reached the end of queue */
			prev->p_nextready = rp;
			rp->p_nextready = temp;
		}
	}
	/*********************************************************
	*                End of EDF Modification               *
	*********************************************************/

	if (cpuid == rp->p_cpu)
	{
		/*
	   * enqueueing a process with a higher priority than the current one,
	   * it gets preempted. The current process must be preemptible. Testing
	   * the priority also makes sure that a process does not preempt itself
	   */
		struct proc *p;
		p = get_cpulocal_var(proc_ptr);
		assert(p);
		if ((p->p_priority > rp->p_priority) &&
			(priv(p)->s_flags & PREEMPTIBLE))
			RTS_SET(p, RTS_PREEMPTED); /* calls dequeue() */
	}
#ifdef CONFIG_SMP
	/*
   * if the process was enqueued on a different cpu and the cpu is idle, i.e.
   * the time is off, we need to wake up that cpu and let it schedule this new
   * process
   */
	else if (get_cpu_var(rp->p_cpu, cpu_is_idle))
	{
		smp_schedule(rp->p_cpu);
	}
#endif

	/* Make note of when this process was added to queue */
	read_tsc_64(&(get_cpulocal_var(proc_ptr)->p_accounting.enter_queue));

#if DEBUG_SANITYCHECKS
	assert(runqueues_ok_local());
#endif
}

/*===========================================================================*
 *				Other Unmodified Code					     * 
 *===========================================================================*/
