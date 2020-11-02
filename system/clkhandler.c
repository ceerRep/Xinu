/* clkhandler.c - clkhandler */

#include <xinu.h>

/*------------------------------------------------------------------------
 * clkhandler - high level clock interrupt handler
 *------------------------------------------------------------------------
 */
void	clkhandler(void)
{
	/* Decrement the ms counter, and see if a second has passed */

	if((++count1000) >= 1000) {

		/* One second has passed, so increment seconds count */

		clktime++;

		/* Reset the local ms counter for the next second */

		count1000 = 0;
	}

	/* Handle sleeping processes if any exist */

	if(!isempty(sleepq)) {

		for (qid16 now = firstid(sleepq), end = queuetail(sleepq); now != end; now = queuetab[now].qnext) {
			queuetab[now].qkey--;
		}

		if((queuetab[firstid(sleepq)].qkey) <= 0) {
			wakeup();
		}
	}

	/* Decrement the preemption counter, and reschedule when the */
	/*   remaining time reaches zero			     */

	if((--preempt) <= 0) {
		preempt = QUANTUM;
		resched();
	}
}
