/* This file contains the main program of the SCHED scheduler. It will sit idle
 * until asked, by PM, to take over scheduling a particular process.
 */

/* The _MAIN def indicates that we want the schedproc structs to be created
 * here. Used from within schedproc.h */
#define _MAIN

#include "sched.h"
#include "schedproc.h"
#include <stdlib.h>
/*#include <time.h>*/

/* Declare some local functions. */
FORWARD _PROTOTYPE( void reply, (endpoint_t whom, message *m_ptr)	);
FORWARD _PROTOTYPE( void sef_local_startup, (void)			);

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
PUBLIC int main(void)
{
	/* Main routine of the scheduler. */
	message m_in;	/* the incoming message itself is kept here. */
	int call_nr;	/* system call number */
	int who_e;	/* caller's endpoint */
	int result;	/* result to system call */
	int rv;

	srand( 5716 );
	last_lucky_one = NULL;

	/* SEF local startup. */
	sef_local_startup();

	/* Initialize scheduling timers, used for running balance_queues */
	init_scheduling();
	{
		int i;
		for(i=0;i<3;++i){
			printf("I printf 3 replicate messages to show that this new sched %s works.\n",__TIME__);
		}
	}

	/* This is SCHED's main loop - get work and do it, forever and forever. */
	while (TRUE) {
		int ipc_status;

		/*printf( "sched running\n" );*/
		/* Wait for the next message and extract useful information from it. */
		if (sef_receive_status(ANY, &m_in, &ipc_status) != OK)
			panic("SCHED sef_receive error");
		who_e = m_in.m_source;	/* who sent the message */
		call_nr = m_in.m_type;	/* system call number */

		/* Check for system notifications first. Special cases. */
		if (is_ipc_notify(ipc_status)) {
			switch(who_e) {
				case CLOCK:
					sched_expire_timers(m_in.NOTIFY_TIMESTAMP);
					/*printf( "sched_expire_timers\n" );*/
					continue;	/* don't reply */
				default :
					result = ENOSYS;
			}

			/*printf( "sched goto sendreply\n" );*/
			goto sendreply;
		}

		/*printf( "sched switch(call_nr)\n" );*/
		switch(call_nr) {
		case SCHEDULING_START:
			result = do_start_scheduling(&m_in);
			break;
		case SCHEDULING_STOP:
			result = do_stop_scheduling(&m_in);
			break;
		case SCHEDULING_SET_NICE:
			result = do_nice(&m_in);
			break;
		case SCHEDULING_NO_QUANTUM:
			/* This message was sent from the kernel, don't reply */
			if (IPC_STATUS_FLAGS_TEST(ipc_status,
				IPC_FLG_MSG_FROM_KERNEL)) {
				if ((rv = do_noquantum(&m_in)) != (OK)) {
					printf("SCHED: Warning, do_noquantum "
						"failed with %d\n", rv);
				}
				continue; /* Don't reply */
			}
			else {
				printf("SCHED: process %d faked "
					"SCHEDULING_NO_QUANTUM message!\n",
						who_e);
				result = EPERM;
			}
			break;
		default:
			result = no_sys(who_e, call_nr);
		}

sendreply:
		/* Send reply. */
		if (result != SUSPEND) {
			m_in.m_type = result;  		/* build reply message */
			reply(who_e, &m_in);		/* send it away */
		}
 	}
	return(OK);
}

/*===========================================================================*
 *				reply					     *
 *===========================================================================*/
PRIVATE void reply(endpoint_t who_e, message *m_ptr)
{
	int s = send(who_e, m_ptr);    /* send the message */
	if (OK != s)
		printf("SCHED: unable to send reply to %d: %d\n", who_e, s);
}

/*===========================================================================*
 *			       sef_local_startup			     *
 *===========================================================================*/
PRIVATE void sef_local_startup(void)
{
	/* No init callbacks for now. */
	/* No live update support for now. */
	/* No signal callbacks for now. */

	/* Let SEF perform startup. */
	sef_startup();
}