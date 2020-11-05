/* resched.c - resched, resched_cntl */

#include <xinu.h>
#include <queue.h>
#include <sche.h>
#include <clock.h>
#include <process.h>

struct	defer	Defer;
extern int proccnt[];
extern int proctype[];

int64 time_used;

void calc_proc_type(pid32 pid)
{
	if (proctype[pid] & 0b100)
    {
        int64 tmp = time_used;
		for (int i = 0; i < NPROC; i++)
		{
			if (proctab[i].prstate == PR_READY)
			{
				if (proctype[i] && !(proctype[i] & 0b100))
				{
					tmp += proctab[i].prtime * ((proctype[i] & 1) ? 1 : -1);
				}
			}
		}

		// kprintf("%d %d\n", (int)tmp, proctype[pid]);

		if (tmp > 0) 
		{
			proctype[currpid] &= ~1;
		}
		else if (tmp < 0)
		{
			proctype[currpid] |= 1;
		}
    }
}

/*------------------------------------------------------------------------
 *  resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void	resched(void)		/* Assumes interrupts are disabled	*/
{
	static bool8 prev_tick_initialized = 0;
	static int64 prev_tick = 0;
	if (prev_tick_initialized)
    {
		int64 now_tick = getticks();
		int64 diff_tick = now_tick - prev_tick;
		diff_tick >>= 10;
		if (proctype[currpid])
            time_used += diff_tick * ((proctype[currpid] & 1) ? 1 : -1);
        if (currpid != NULLPROC)
        {
            proctab[currpid].prtime -= diff_tick;
            if (proctab[currpid].prtime < 0)
                proctab[currpid].prtime = 0;
        }
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
			time_used = 0;

			pid32 pid;
			qid16 now = firstid(readylist);

			for (pid = 0; pid < NPROC; pid++)
				if (proctab[pid].prstate != PR_FREE && proctab[pid].prprio > 0)
					proctab[pid].prprio = 40 - proctab[pid].prprio;

			do {
				pid = now;
				proctab[pid].prtime = timeslice_from_priority(proctab[pid].prprio);
				queuetab[now].qkey = proctab[pid].prtime;
				now = queuetab[now].qnext;
			} while (queuetail(readylist) != now);
		}

		// Insertion Sort
		// Not need ?

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

	// kprintf("Resched to [%d] %s\n", currpid, proctab[currpid].prname);

	ptnew = &proctab[currpid];
	ptnew->prstate = PR_CURR;
	preempt = QUANTUM;		/* Reset time slice for process	*/

	calc_proc_type(currpid);
	prev_tick = getticks();
	prev_tick_initialized = 1;

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
