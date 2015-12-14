/*
 *	A T T C P _ T I M E R . C
 *
 * Test TCP connection.  Makes a connection on port 55000
 * and transfers fabricated buffers or data copied from stdin.
 *
 *	T I M E R - time related functions
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

static void tvadd();
static void tvsub();

void
delay(us)
{
        struct timeval tv;

        tv.tv_sec = 0;
        tv.tv_usec = us;
        (void)select( 1, (fd_set*)0, (fd_set *)0, (fd_set *)0, &tv );
}

/*
 *			TIMER - Start/Stop {0|1}
 */
static ended = 0;
void
timer0()
{
/*
 * init statistics structures - permit restart when using udp
 */
#ifdef HAVE_PERFSTAT
	perfstat_init();
	perfstat_start();
#endif

	gettimeofday(&time0, (struct timezone *)0);
	getrusage(RUSAGE_SELF, &ru0);
	ended = 0;
}

void
timer1()
{
/*
 * initial libperfstat interface
 */
	if (ended++)
		return;
#ifdef HAVE_PERFSTAT
	perfstat_stop();
#endif
	gettimeofday(&time1, (struct timezone *)0);
	getrusage(RUSAGE_SELF, &ru1);
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
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

double
time_real()
{
	struct timeval td;

	timer1();

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

	timer1();

	/* Get CPU time (user+sys) */
	tvadd( &t0, &ru0.ru_utime, &ru0.ru_stime );
	tvadd( &t1, &ru1.ru_utime, &ru1.ru_stime );
	tvsub( &td, &t1, &t0 );
	cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
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
	et = td.tv_sec + ((double)td.tv_usec) / 1000000;

	return( et );
}
