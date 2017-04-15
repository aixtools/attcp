/*
 *	A T T C P _ P T H R E A D . C
 * pthread management
 *
 * Test TCP connection.  Makes a connection on port 32765, 32766, 32767
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Copyright 2013-2017: Michael Felt and AIXTOOLS.NET
 *

 * $Date: 2016-01-17 19:09:44 +0000 (Sun, 17 Jan 2016) $
 * $Revision: 179 $
 * $Author: michael $
 * $Id: attcp_pthread.c 179 2016-01-17 19:09:44Z michael $
 */
#include <config.h>

#include "attcp.h"

/*
 * static | global variables to manage threads
 */
int pthreads_created;
int *return_codes;
pthread_t	*pthreads;
attcp_conn_p	attcp_conn_data;

/*
 * work - thread work management
 */

void *
attcp_work(void * data)
{
        attcp_conn_p	c   = (attcp_conn_p) data;
        int		tid = c->tid;

#ifdef VERBOSE
        fprintf(stderr,"attcp_work: %d started\n", c->tid);
        fflush(stderr);
#endif

        main_work(c);

#ifdef VERBOSE
        fprintf(stderr,"attcp_work: %d finished\n", c->tid);
        fflush(stderr);
#endif
        c->done = 1;
        pthread_exit(NULL);
}

/*
 * start - start the threads
 */
attcp_pthread_start(int threads)
{
/*
 * also set all relevant signals to IGNORE
 * as I do not wish the threads to ALL respond to a signal
 * INSTEAD - the main thread will be setup to respond
 * and let the threads end "gracefully"
 * and still get a report of the connection status
 */
	attcp_conn_t	*connection = attcp_conn_data;
	pthread_t	*thread     = pthreads;

	int cnt = threads;
	int *rcs = return_codes;

	for (; cnt--; rcs++, thread++, connection++)	{
		*rcs = pthread_create(thread, NULL, attcp_work, (void *) connection++);
		if (*rcs == 0)	{
			connection->tid = *thread;
			pthread_detach(*thread);
		} else
			err("pthread create failed");
	}
}

/*
 * init - initialize the process thread management structures
 */
boolean
attcp_pthread_init(int threads)
{
	return_codes	= (int *) malloc(threads * sizeof(int));
	pthreads	= (pthread_t *) malloc(threads * sizeof(pthread_t));
	attcp_conn_data =
		(attcp_conn_p) malloc(threads * sizeof(attcp_conn_t));

	return ((return_codes		!= NULL) &&
		(pthreads		!= NULL) &&
		(attcp_conn_data	!= NULL));
}

/*
 * routine to report thread statistics when xfer routines finish
 */
attcp_thread_report()
{
}
