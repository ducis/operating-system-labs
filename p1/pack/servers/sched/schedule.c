/* This file contains the scheduling policy for SCHED
 *
 * The entry points are:
 *   do_noquantum:        Called on behalf of process' that run out of quantum
 *   do_start_scheduling  Request from PM to start scheduling a proc
 *   do_stop_scheduling   Request from PM to stop scheduling a proc
 *   do_nice		  Request from PM to change the nice level on a proc
 *   init_scheduling      Called from main.c to set up/prepare scheduling
 */

#include "sched.h"
#include "schedproc.h"
#include <minix/com.h>
#include <machine/archtypes.h>
#include "kernel/proc.h" /* for queue constants */
#include "kernel/const.h"
#include "kernel/priv.h"
#include <minix/syslib.h>
#include <stdlib.h>
#include "assert.h"


PRIVATE timer_t sched_timer;
PRIVATE unsigned balance_timeout;

#define BALANCE_TIMEOUT	5 /* how often to balance queues in seconds */

FORWARD _PROTOTYPE( int schedule_process, (struct schedproc * rmp)	);
FORWARD _PROTOTYPE( void balance_queues, (struct timer *tp)		);

#define DEFAULT_USER_TIME_SLICE 200

/*#define SET_LOT_TIMER sched_set_timer(&sched_timer, sys_hz(), timed_draw_lots, 0);*/

PRIVATE int draw_lots(struct schedproc *bad){
	int rv, proc_nr_n;
	int total = 0,which;
	register struct schedproc *itr = 0;
	/*printf( "==============draw_lots()\n" );*/
#define TRAVERSE for (proc_nr_n=0, itr=schedproc; proc_nr_n < NR_PROCS; ++proc_nr_n, ++itr)
#define IF_GOOD if( (itr->flags & LOTTERIED) && (itr!=bad) )
#define SET_LUCK( P,L )     rv = sys_schedule((P)->endpoint,(L),(P)->time_slice); if( rv!=OK ) { printf( "failed to sys_schedule() at line %d\n", __LINE__ ); return rv; }
#define LUCK(P)    SET_LUCK( P , LUCKY_Q )
#define UNLUCK(P)  SET_LUCK( P , UNLUCKY_Q )
	TRAVERSE{
		IF_GOOD{
			assert(itr->flags & IN_USE);
			total+=itr->priority;
		}
	}
	if(total){
		if(bad&&(bad->flags & LOTTERIED)){
			assert(bad->flags & IN_USE);
			UNLUCK(bad);
		}
		which = (rand()*(RAND_MAX+1)+rand())%total;
		TRAVERSE{
			IF_GOOD{
				assert(itr->flags & IN_USE);
				if(which<itr->priority){
					break;
				}else{
					which-=itr->priority;
				}
			}
		}
		assert(itr);
		assert(itr<schedproc+NR_PROCS);
		assert(itr->flags & LOTTERIED);
		assert(itr->flags & IN_USE);
		if(last_lucky_one && (last_lucky_one != itr)){
			UNLUCK(last_lucky_one);
			last_lucky_one = NULL;
		}
		LUCK(itr);
		last_lucky_one = itr;
	}else{
		if(bad&&(bad->flags & LOTTERIED)){
			assert(bad->flags & IN_USE);
			LUCK(bad);
		}
	}
	return OK;
}

/*===========================================================================*
 *				do_noquantum				     *
 *===========================================================================*/

PUBLIC int do_noquantum(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n;

	if (sched_isokendpt(m_ptr->m_source, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg %u.\n",
		m_ptr->m_source);
		return EBADSRCDST;
	}

	rmp = &schedproc[proc_nr_n];

	if( OK != (rv = draw_lots(m_ptr->SCHEDULING_QUANTUM?NULL:rmp)) ){
		return rv;
	}

	if(rmp->flags & LOTTERIED){
		return OK;
	}
	if (rmp->priority < MIN_USER_Q) {
		rmp->priority += 1; /* lower priority */
	}

	if ((rv = schedule_process(rmp)) != OK) {
		return rv;
	}
	return OK;
}

/*===========================================================================*
 *				do_stop_scheduling			     *
 *===========================================================================*/
PUBLIC int do_stop_scheduling(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n;

	/* Only accept stop messages from PM */
	if (!is_from_pm(m_ptr))
		return EPERM;

	if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg %u.\n",
		m_ptr->SCHEDULING_ENDPOINT);
		return EBADSRCDST;
	}

	rmp = &schedproc[proc_nr_n];
	rmp->flags = 0; /*&= ~IN_USE;*/

	return OK;
}

/*===========================================================================*
 *				do_start_scheduling			     *
 *===========================================================================*/
PUBLIC int do_start_scheduling(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n, parent_nr_n;

	/*printf("sched run %d\n",__LINE__);*/

	/* Only accept start messages from PM */
	if (!is_from_pm(m_ptr))
		return EPERM;

	/*printf("sched run %d\n",__LINE__);*/

	/* Resolve endpoint to proc slot. */
	if ((rv = sched_isemtyendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n))
			!= OK) {
		return rv;
	}
	rmp = &schedproc[proc_nr_n];

	/*printf("sched run %d\n",__LINE__);*/

	/* Populate process slot */
	rmp->endpoint     = m_ptr->SCHEDULING_ENDPOINT;
	rmp->parent       = m_ptr->SCHEDULING_PARENT;
	rmp->nice         = m_ptr->SCHEDULING_NICE;

	/* Find maximum priority from nice value */
	rv = nice_to_priority(rmp->nice, &rmp->max_priority);
	if (rv != OK)
		return rv;

	/*printf("sched run %d\n",__LINE__);*/

	/* Inherit current priority and time slice from parent. Since there
	 * is currently only one scheduler scheduling the whole system, this
	 * value is local and we assert that the parent endpoint is valid */
	if (rmp->endpoint == rmp->parent) {
		/* We have a special case here for init, which is the first
		   process scheduled, and the parent of itself. */
		rmp->priority   = USER_Q;
		rmp->time_slice = DEFAULT_USER_TIME_SLICE;

	}
	else {
		if ((rv = sched_isokendpt(m_ptr->SCHEDULING_PARENT,
				&parent_nr_n)) != OK)
			return rv;

		rmp->priority = schedproc[parent_nr_n].priority;
		rmp->time_slice = schedproc[parent_nr_n].time_slice;
	}

	/*printf("sched run %d\n",__LINE__);*/

	/* Take over scheduling the process. The kernel reply message populates
	 * the processes current priority and its time slice */
	if ((rv = sys_schedctl(rmp->endpoint)) != OK) {
		printf("Sched: Error overtaking scheduling for %d, kernel said %d\n",
			rmp->endpoint, rv);
		return rv;
	}
	rmp->flags = IN_USE;

	/*printf("sched run %d\n",__LINE__);*/


	{/*LOTTERY*/
		/*int process_number;
		if(!isokendpt(m_ptr->SCHEDULING_ENDPOINT, &process_number)) {
			return EINVAL;
		}
		if(!( priv(proc_addr(process_number))->s_flags & SYS_PROC )){*/
		struct priv s;
		if(OK!=(rv = sys_getpriv(&s, m_ptr->SCHEDULING_ENDPOINT))){
			printf("sched can't get the priv of ep=%d\n",m_ptr->SCHEDULING_ENDPOINT);
			return rv;
		}
		if(!( s.s_flags & SYS_PROC )){
			rmp->flags |= LOTTERIED;
			rmp->priority = 5;
			rmp->max_priority = -1;
			rmp->time_slice = DEFAULT_USER_TIME_SLICE/*30000*/;
			rv = sys_schedule(rmp->endpoint, UNLUCKY_Q, rmp->time_slice);
			m_ptr->SCHEDULING_SCHEDULER = SCHED_PROC_NR;
			/*printf("+++++++++sched uses lottery scheduling on ep=%d\n",m_ptr->SCHEDULING_ENDPOINT);*/
			return rv;
		}/*else{
			printf("-_-sched doesn't use lottery scheduling on ep=%d\n",m_ptr->SCHEDULING_ENDPOINT);
		}*/
	}


	/* Schedule the process, giving it some quantum */
	if ((rv = schedule_process(rmp)) != OK) {
		printf("Sched: Error while scheduling process, kernel replied %d\n",
			rv);
		return rv;
	}

	/* Mark ourselves as the new scheduler.
	 * By default, processes are scheduled by the parents scheduler. In case
	 * this scheduler would want to delegate scheduling to another
	 * scheduler, it could do so and then write the endpoint of that
	 * scheduler into SCHEDULING_SCHEDULER
	 */

	m_ptr->SCHEDULING_SCHEDULER = SCHED_PROC_NR;

	return OK;
}

/*===========================================================================*
 *				do_nice					     *
 *===========================================================================*/
PUBLIC int do_nice(message *m_ptr)
{
	struct schedproc *rmp;
	int rv;
	int proc_nr_n;
	int nice;
	unsigned new_q, old_q, old_max_q;
	int old_nice;

	/* Only accept nice messages from PM */
	if (!is_from_pm(m_ptr))
		return EPERM;

	if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg %u.\n",
		m_ptr->SCHEDULING_ENDPOINT);
		return EBADSRCDST;
	}

	rmp = &schedproc[proc_nr_n];
	nice = m_ptr->SCHEDULING_NICE;

	if (rmp->flags & LOTTERIED){
		int hi = 100;
		int lo = 1;
		int p = rmp->priority+nice;
		if(p>hi){
			p = hi;
		}else if(p<lo){
			p = lo;
		}
		rmp->priority = p;
		return OK;
	}
	if ((rv = nice_to_priority(nice, &new_q)) != OK)
		return rv;

	/* Store old values, in case we need to roll back the changes */
	old_q     = rmp->priority;
	old_max_q = rmp->max_priority;
	old_nice  = rmp->nice;

	/* Update the proc entry and reschedule the process */
	rmp->max_priority = rmp->priority = new_q;
	rmp->nice = nice;

	if ((rv = schedule_process(rmp)) != OK) {
		/* Something went wrong when rescheduling the process, roll
		 * back the changes to proc struct */
		rmp->priority     = old_q;
		rmp->max_priority = old_max_q;
		rmp->nice         = old_nice;
	}

	return rv;
}

/*===========================================================================*
 *				schedule_process			     *
 *===========================================================================*/
PRIVATE int schedule_process(struct schedproc * rmp)
{
	int rv;

	if ((rv = sys_schedule(rmp->endpoint, rmp->priority,
			rmp->time_slice)) != OK) {
		printf("SCHED: An error occurred when trying to schedule %d: %d\n",
		rmp->endpoint, rv);
	}

	return rv;
}


/*===========================================================================*
 *				start_scheduling			     *
 *===========================================================================*/

PUBLIC void init_scheduling(void)
{
	balance_timeout = BALANCE_TIMEOUT * sys_hz();
	tmr_inittimer(&sched_timer);
	sched_set_timer(&sched_timer, balance_timeout, balance_queues, 0);
	/*SET_LOT_TIMER;*/
}

/*===========================================================================*
 *				balance_queues				     *
 *===========================================================================*/

/* This function in called every 100 ticks to rebalance the queues. The current
 * scheduler bumps processes down one priority when ever they run out of
 * quantum. This function will find all proccesses that have been bumped down,
 * and pulls them back up. This default policy will soon be changed.
 */
PRIVATE void balance_queues(struct timer *tp)
{
	struct schedproc *rmp;
	int proc_nr;
	int rv;

	for (proc_nr=0, rmp=schedproc; proc_nr < NR_PROCS; proc_nr++, rmp++) {
		if (rmp->flags & IN_USE) {
			if(rmp->flags & LOTTERIED) continue;
			if (rmp->priority > rmp->max_priority) {
				rmp->priority -= 1; /* increase priority */
				schedule_process(rmp);
			}
		}
	}

	sched_set_timer(&sched_timer, balance_timeout, balance_queues, 0);
}
