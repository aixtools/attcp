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
 * check connections for io_done status
 * return when finished
 */
void
attcp_thread_stop(int threads)
{
	attcp_conn_p	c	= attcp_conn_data;
	int	idx = 0;
	uint64_t	nbytes = 0;

	for(; threads--; c++)
		c->io_done++;
}

/*
 * check connections for io_done status
 * return when finished
 */
uint64_t
attcp_thread_done(int threads)
{
	attcp_conn_p	c	= attcp_conn_data;
	int	idx = 0;
	uint64_t	nbytes = 0;

	while (threads--) {
		while (!c->nbytes) {
#ifdef VERBOSE
			fprintf(stderr,".");
#endif
			usleep(100);
		}
#ifdef VERBOSE
		fprintf(stderr,"idx:%d->C:%X sd:%d done:%d bytes:%lld\n", idx, c, c->sd, c->io_done, c->nbytes);
#endif
		sockCalls += c->sockcalls;
		nbytes += c->nbytes;
		c++;
		idx++;
	}
	return(nbytes);
}

/*
 * work - thread work management
 */

void *
attcp_work(void * data)
{
        attcp_conn_p	c   = (attcp_conn_p) data;
        int		tid = c->tid;

#ifdef VERBOSE
        fprintf(stderr,"START_work: C:%08X %d sd:%d started\n",
		c, c->tid, c->sd);
        fflush(stderr);
#endif

        attcp_xfer(c);

#ifdef VERBOSE
        fprintf(stderr,"STOP__work: C:%08X %d sd:%d finished\n",
		c, c->tid, c->sd);
        fflush(stderr);
#endif
        c->io_done = 1;
        pthread_exit(NULL);
}

/*
 * start - start the threads
 */
pthread_t
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

	int cnt = threads - 1;
	int *rcs = return_codes;

	/*
	 * start the global timer
	 */
	timer0(RUSAGE_SELF, NULL);
#ifdef VERBOSE
	fprintf(stderr,"1. t: %08lx C:%08X\n", thread, connection);
#endif
	while (cnt--)	{
#ifdef VERBOSE
		fprintf(stderr,"F. c: %08lx T:%08X", thread, connection);
		fprintf(stderr," sd:%d *t:%d *t:%ld\n",
			connection->sd, *thread, *thread);
		fflush(stderr);
#endif
		*rcs = pthread_create(thread, NULL,
			attcp_work, (void *) connection);
		if (*rcs == 0)	{
			connection->tid = *thread;
			if (cnt)
				pthread_detach(*thread);
		} else
			err("pthread create failed");
#ifdef VERBOSE
		fprintf(stderr,"T. c: %08lx T:%08X", thread, connection);
		fprintf(stderr," sd:%d *t:%d *t:%ld\n",
			connection->sd, *thread, *thread);
		fflush(stderr);
#endif
		rcs++; thread++; connection++;
	}
	/*
	 * the last thread is not detached - return it's id
	 */
#ifdef VERBOSE
	fprintf(stderr,"L. c: %08lx T:%08X", thread, connection);
	fprintf(stderr," sd:%d *t:%d *t:%ld\n",
		connection->sd, *thread, *thread);
	fflush(stderr);
#endif
	*rcs = pthread_create(thread, NULL, attcp_work, (void *) connection);
#ifdef VERBOSE
	fprintf(stderr,"D. c: %08lx T:%08X", thread, connection);
	fprintf(stderr," sd:%d *t:%d *t:%ld\n",
		connection->sd, *thread, *thread);
	fflush(stderr);
#endif
	if (*rcs == 0)
		connection->tid = *thread;
	return(*thread);
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

	bzero((char *)return_codes, threads * sizeof(int));
	bzero((char *)pthreads, threads * sizeof(pthread_t));
	bzero((char *)attcp_conn_data, threads * sizeof(attcp_conn_t));

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
/*
 * create the threads and set the options
 */
void
attcp_pthread_socket(attcp_opt_p a_opts)
{
	int		threads = a_opts->threads;
	attcp_conn_p	c	= attcp_conn_data;
	attcp_set_p	cs;
	
	for (; threads--; c++) {
		c->settings = a_opts->settings;
		cs = &c->settings;
#ifdef VERBOSE
		fprintf(stderr,"nbytes: %ld\n", c->nbytes);
#endif
		if ((cs->transport == SOCK_DGRAM) && cs->io_size < 5) {
			/* send more than the sentinel size */
			cs->io_size = 5;
		}
		attcp_socket(c, a_opts);
		attcp_setoption(c);
		c->buf = (char *)malloc(cs->io_size);
		if (c->buf == NULL)
			err("malloc in attcp_thread_socket");
		c->io_done = 0;
		attcp_connect(c);
	}
}
