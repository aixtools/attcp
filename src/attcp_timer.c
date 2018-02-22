/*
 *	A T T C P _ T I M E R . C
 *	T I M E R - time related functions
 *
 * Test TCP connection.  Makes a connection on port 32765, 32766, 32767
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2017: Michael Felt and AIXTOOLS.NET
 *
 * $Date: 2017-04-12 18:37:47 +0000 (Wed, 12 Apr 2017) $
 * $Revision: 239 $
 * $Author: michael $
 * $Id: attcp_timer.c 239 2017-04-12 18:37:47Z michael $
 */
#include <config.h>
#include "attcp.h"

void prep_timer();
double read_timer();
double elapsed_time();

static struct	timeval time0;	/* Time at which timing started */
static struct	timeval time1;	/* Time at which timing stopped */
static struct	rusage ru0;	/* Resource utilization at the start */
static struct	rusage ru1;	/* Resource utilization at the end */

/*
 * I doubt if select() on fd(1) - stdout - is the correct approach to
 * pause XXX microseconds, need an improved "sleep" aka delay mechinism.
 */
void
delay(int _usec)
{
        struct timeval tv;

        tv.tv_sec = 0;
        tv.tv_usec = _usec;
        (void)select( 1, (fd_set*)0, (fd_set *)0, (fd_set *)0, &tv );
}

static void
tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec >= 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

/*
 *			TIMER - Start/Stop {0|1}
 */
static int ended = 0;
void
timer0(int Who, attcp_conn_p c)
{
/*
 * init statistics structures - permit restart when using udp
 */
	if (Who == RUSAGE_SELF) {
#ifdef HAVE_LIBPERFSTAT
		perfstat_init();
		perfstat_start();
#endif
		gettimeofday(&time0, (struct timezone *)0);
		getrusage(RUSAGE_SELF, &ru0);
		ended = 0;
	}
	else
		getrusage(Who, &c->ru0);
}

void
timer1(int Who, attcp_conn_p c)
{
/*
 * initial libperfstat interface
 */
	if (Who == RUSAGE_SELF) {
		if (ended++)
			return;
		getrusage(RUSAGE_SELF, &ru1);
		gettimeofday(&time1, (struct timezone *)0);
#ifdef HAVE_LIBPERFSTAT
		perfstat_stop();
#endif
	} else
		getrusage(Who, &c->ru1);
}

double
time_real()
{
        struct timeval td;

        timer1(RUSAGE_SELF, NULL);

        /* Calculate real time */
        tvsub( &td, &time1, &time0 );
        return( td.tv_sec + ((double)td.tv_usec) / 1000000);
}

double
time_busy()
{
        struct timeval td;
        struct timeval t0, t1;
        double cput = 0.0;

        timer1(RUSAGE_SELF, NULL);

        /* Get CPU time (user+sys) */
        tvadd( &t0, &ru0.ru_utime, &ru0.ru_stime );
        tvadd( &t1, &ru1.ru_utime, &ru1.ru_stime );
        tvsub( &td, &t1, &t0 );
        cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
        if( cput < 0.00001 )  cput = 0.00001;
        return( cput );
}

double
time_elasped()
{
	struct timeval td;

	timer1(RUSAGE_SELF, NULL);

	/* Calculate real time elasped */
	tvsub( &td, &time1, &time0 );
	return( ((td.tv_sec * 1000000) + td.tv_usec) / (double) 1000000.0);
}

double
time_consumed()
{
	struct timeval td;
	struct timeval t0, t1;
	double cput = 0.0;

	timer1(RUSAGE_SELF, NULL);

	/* Get CPU (physc) time (user+sys) */
	tvadd( &t0, &ru0.ru_utime, &ru0.ru_stime );
	tvadd( &t1, &ru1.ru_utime, &ru1.ru_stime );
	tvsub( &td, &t1, &t0 );
	cput = ((td.tv_sec * 1000000) + td.tv_usec) / (double) 1000000.0;
	if( cput < 0.00001 )  cput = 0.00001;
	return( cput );
}

double elapsed_time()
{
	struct timeval timedol;
	struct timeval td;
	double et = 0.0;

	gettimeofday(&timedol, (struct timezone *)0);
	tvsub( &td, &timedol, &time0 );
	et = ((td.tv_sec * 1000000) + td.tv_usec) / (double) 1000000.0;
	return( et );
}

#ifdef TESTX
main()
{
	timer0(RUSAGE_SELF, NULL);
	sleep(1);
	printf("%8.2f\n", elapsed_time());
}
#endif
