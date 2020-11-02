/* resched.c - resched, resched_cntl */

#include <xinu.h>
#include <queue.h>
#include <sche.h>
#include <process.h>

struct	defer	Defer;
extern int proccnt[];

/*------------------------------------------------------------------------
 *  resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void	resched(void)		/* Assumes interrupts are disabled	*/
{
	if (currpid != NULLPROC) {
		proctab[currpid].prtime -= QUANTUM;
		if (proctab[currpid].prtime < 0)
			proctab[currpid].prtime = 0;
	}

	struct procent *ptold;	/* Ptr to table entry for old process	*/
	struct procent *ptnew;	/* Ptr to table entry for new process	*/

	/* If rescheduling is deferred, record attempt and return */

	if (Defer.ndefers > 0) {
		Defer.attempt = TRUE;
		return;
	}

	/* Point to process table entry for the current (old) process */

	ptold = &proctab[currpid];

	if (ptold->prstate == PR_CURR) {  /* Process remains eligible */

		/* Old process will no longer remain current */

		ptold->prstate = PR_READY;
		insert(currpid, readylist, ptold->prtime);
	}

	/* Check time slice */
	{
		if (proctab[firstid(readylist)].prtime == 0) {
			pid32 pid;
			qid16 now = firstid(readylist), last = lastid(readylist);

			do {
				pid = now;
				proctab[pid].prtime = timeslice_from_priority(proctab[pid].prprio);
				queuetab[now].qkey = proctab[pid].prtime;
				now = queuetab[now].qnext;
			} while (last != now);
		}

		// Insertion Sort
		// Not need

		qid16 now = firstid(readylist);
		firstid(readylist) = queuetail(readylist);
		lastid(readylist) = queuehead(readylist);

		while (now != queuetail(readylist)) {
			qid16 tmp = queuetab[now].qnext;
			insert(now, readylist, proctab[now].prtime);
			now = tmp;
		}
	}
	/* Force context switch to highest time ready process */
	currpid = dequeue(readylist);

	kprintf("Resched to [%d] %s\n", currpid, proctab[currpid].prname);
	for(int i = 0; i < 16; i++)
		if (proctab[i].prstate != PR_FREE)
			kprintf("%s [%d] %d\n", proctab[i].prname, i, proccnt[i]);

	ptnew = &proctab[currpid];
	ptnew->prstate = PR_CURR;
	preempt = QUANTUM;		/* Reset time slice for process	*/

	ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

	/* Old process returns here when resumed */

	return;
}

/*------------------------------------------------------------------------
 *  resched_cntl  -  Control whether rescheduling is deferred or allowed
 *------------------------------------------------------------------------
 */
status	resched_cntl(		/* Assumes interrupts are disabled	*/
	  int32	defer		/* Either DEFER_START or DEFER_STOP	*/
	)
{
	switch (defer) {

	    case DEFER_START:	/* Handle a deferral request */

		if (Defer.ndefers++ == 0) {
			Defer.attempt = FALSE;
		}
		return OK;

	    case DEFER_STOP:	/* Handle end of deferral */
		if (Defer.ndefers <= 0) {
			return SYSERR;
		}
		if ( (--Defer.ndefers == 0) && Defer.attempt ) {
			resched();
		}
		return OK;

	    default:
		return SYSERR;
	}
}
