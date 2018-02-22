/*
 *      A T T C P
 *
 * Test TCP connection.  Makes a connection on port 32765, 32766, 32767
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2016: Michael Felt and AIXTOOLS.NET
 *

 * $Date: 2016-01-17 19:09:44 +0000 (Sun, 17 Jan 2016) $
 * $Revision: 179 $
 * $Author: michael $
 * $Id: attcp_perfstat.c 179 2016-01-17 19:09:44Z michael $
 *
 * modified to work with attcp and report end statistics
 */
/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos61N src/bos/usr/ccs/lib/libperfstat/simpleifstat.c 1.3.1.2          */
/*                                                                        */
/* Licensed Materials - Property of IBM                                   */
/*                                                                        */
/* Restricted Materials of IBM                                            */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2008,2010              */
/* All Rights Reserved                                                    */
/*                                                                        */
/* US Government Users Restricted Rights - Use, duplication or            */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.      */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/*
static char sccsid[] = "@(#)65        1.3.1.2  src/bos/usr/ccs/lib/libperfstat/simpleifstat.c, libperfstat, bos61N, 1031A_61N 7/21/10 04:54:00";
 */
/* The sample program display the metrics *
 * related to each and every Individual   * 
 * network interface in the LPAR          */
#include <config.h>
#ifdef HAVE_LIBPERFSTAT
#include "attcp.h"

#include <libperfstat.h>
#include <net/if_types.h>
typedef	int boolean;
static void pru64(u_longlong_t d, char *label, char *star);

/* Check value returned by malloc for NULL */

#define CHECK_FOR_MALLOC_NULL(X) {  if ((X) == NULL) {\
                                       perror ("malloc");\
                                       exit(2);\
                                     }\
				  }

int tot;
int returncode;

/* store the data structures */

static perfstat_netinterface_t *statp ,*statq;
static perfstat_netinterface_total_t *pTotal1, *pTotal0;

/* support for remote node statistics collection in a cluster environment */
/*
perfstat_id_node_t nodeid;
static char nodename[MAXHOSTNAMELEN] = "";
*/
perfstat_id_t nodeid;
static char nodename[IDENTIFIER_LENGTH] = "";
static int collect_remote_node_stats = 0;

/* 
 * NAME: decode 
 *       to determine the type of interface 
 *
 */

char * decode(uchar type) {

    switch(type) {

    case IFT_LOOP:
        return("loopback");

    case IFT_ISO88025:
        return("token-ring");

    case IFT_ETHER:
        return("ethernet");
    }

    return("other");
}

/*
 * NAME: perfstat_init
 *       This function initializes the data structues.
 *       It also collects initial set of values.
 * 
 * RETURNS:
 * On successful completion:
 *   - returns 0.
 * In case of error
 *    - exits with code 1.
 */

int perfstat_init(void)
{
   static done=0;
   if (done++)
	return(0);
    /* check how many perfstat_netinterface_t structures are available */
   tot = perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);

   if (tot == 0) {
        printf("There is no net interface\n");
        exit(0);
   }
   if (tot < 0) {
        perror("perfstat_netinterface: tot<0 ");
        exit(1);
   }
   
   /* allocate enough memory for all the structures */
    
   statp = (perfstat_netinterface_t *)malloc(tot * sizeof(perfstat_netinterface_t));
   CHECK_FOR_MALLOC_NULL(statp);

   statq = (perfstat_netinterface_t *)malloc(tot * sizeof(perfstat_netinterface_t));
   CHECK_FOR_MALLOC_NULL(statq);
   
   pTotal1 = (perfstat_netinterface_total_t *) malloc(sizeof(perfstat_netinterface_total_t));
   CHECK_FOR_MALLOC_NULL(pTotal1);

   pTotal0 = (perfstat_netinterface_total_t *) malloc(sizeof(perfstat_netinterface_total_t));
   CHECK_FOR_MALLOC_NULL(pTotal0);  
   
   return(0);
}

/*
 *Name: display_metrics
 *       collect the metrics and display them
 *
 */
void header_metrics(boolean verbose)
{

    /*
     * Name    - name of the netinterface
     * Type    - type of the interface
     * Ipackets- number of input packets
     * Ierrors - number of input errors
     * Ibytes  - number of input bytes
     * Opackets- number of output packets
     * Oerrors - number of output errors
     * Obytes  - number of output bytes
     *
     */
 
    if (verbose) {
	fprintf(stdout,"LIBPERFSTAT API Interface Stats\n"); 
	fprintf(stdout,"%8s %10s %10s %10s %10s %6s %6s %6s\n",
		"MB/s", "I KB", "O KB", "Ipackets","Opackets","Isize","Osize", "Name");
	fprintf(stdout,"%8s %10s %10s %10s %10s %6s %6s %6s\n",
		"========", "========", "=========", "=======", "========","=====","=====","====");
    }
}

#ifdef USEALARM
boolean my_alarm = FALSE;


void sigalarm( int signal )
{
	extern	boolean	my_alarm;

	my_alarm = TRUE;
}
#endif

void perfstat_start()
{
    /* ask to get all the structures available in one call */
    /* return code is number of structures returned */
    /*
     * Name    - name of the netinterface
     * Type    - type of the interface
     * Ipackets- number of input packets
     * Ierrors - number of input errors
     * Ibytes  - number of input bytes
     * Opackets- number of output packets
     * Oerrors - number of output errors
     * Obytes  - number of output bytes
     *
     */
    perfstat_id_t first;
    int ret, i;
	 	
    /*Retrieving the metrics of Global network interfaces*/
    if (perfstat_netinterface_total( NULL, pTotal0, sizeof(perfstat_netinterface_total_t), 1) < 0){
        perror("perfstat_netinterface_total :");
        exit(1);
    }
    strcpy(first.name , FIRST_NETINTERFACE);
    ret = perfstat_netinterface( &first, statq, sizeof(perfstat_netinterface_t), tot);

    if (ret < 0) {
        perror("perfstat_netinterface: ");
        exit(1);
    }
#ifdef USEALARM
    signal(SIGALRM, sigalarm);
    alarm(1);
	pru64(pTotal0->ibytes,"ib","start total ibytes\n");
#endif
}

void perfstat_stop()
{
	static int stopped = 0;
	if (stopped++)
		return;

	alarm(0);	/* stop the alarms */

	/* get the total netinterface structure */
	if (perfstat_netinterface_total( NULL, pTotal1, sizeof(perfstat_netinterface_total_t), 1) < 0){
		perror("perfstat_netinterface_total :");
		exit(1);
	}
#ifdef USEALARM
	pru64(pTotal1->ibytes,"ib","stop");
	pru64(pTotal0->ibytes,"ib","start\n");
	pru64(pTotal1->obytes,"ob","stop");
	pru64(pTotal0->obytes,"ob","start\n");
	pru64(pTotal1->ipackets,"ip","stop");
	pru64(pTotal0->ipackets,"ip","start\n");
	pru64(pTotal1->opackets,"op","stop");
	pru64(pTotal0->opackets,"op","start\n");
#endif
}

#ifdef USEALARM
void perfstat_alarm(void)
{
	static u_longlong_t x = 0L;
    perfstat_id_t first;
	int i = 1;

	my_alarm = FALSE;
	signal(SIGALRM, sigalarm);
	alarm(1);

    strcpy(first.name , FIRST_NETINTERFACE);
    perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), tot);
	if (perfstat_netinterface_total( NULL, pTotal1, sizeof(perfstat_netinterface_total_t), 1) < 0){
		perror("ALARM: perfstat_netinterface_total :");
		exit(1);
	}
	if (x == 0)
		x  =  statq[i].ibytes;

	pru64(pTotal1->ibytes,"ib","total now");
	pru64(x,"x0",statp[i].name);
	pru64(statp[i].ibytes,"x1","total now");

	x = statp[i].ibytes - x;
	pru64(x,"x2",statp[i].name);
	x = pTotal1->ibytes;

	x = statp[i].ibytes;
/*
	pru64(pTotal1->ipackets,"ip","alarm\n");
*/
}
#endif

#ifdef USEALARM
static void pru64(u_longlong_t d, char *label, char *star)
{
	union _us {
		u_longlong_t u;
		struct {
			unsigned long h;
			unsigned long b;
		}	s;
	}	*ds;
	ds = (union _us *) &d;

	fprintf(stdout, "%s:dbl:%016llX high:%08lX low:%08lx %012llu %s\n", label, d, ds->s.h, ds->s.b, d, star);
}
#endif

static u_longlong_t sub64(u_longlong_t p, u_longlong_t q, char *label)
{
	u_longlong_t d;
	union _us {
		u_longlong_t u;
		struct {
			unsigned long h;
			unsigned long b;
		}	s;
	}	*ps, *qs, *ds;
	ps = (union _us *) &p;
	qs = (union _us *) &q;
	ds = (union _us *) &d;

	d = p - q;

#ifdef	XVERBOSE
	fprintf(stdout, "%s:ps:dbl:%016llX high:%08lX low:%08lx %012u\n", label, p, ps->s.h, ps->s.b, ps->s.b);
	fprintf(stdout, "%s:pq:dbl:%016llX high:%08lX low:%08lx %012u\n", label, q, qs->s.h, qs->s.b, qs->s.b);
	fprintf(stdout, "%s:di:dbl:%016llX high:%08lX low:%08lx %012u #\n", label, d, ds->s.h, ds->s.b, ds->s.b);
#endif
#ifdef USEALARM
	pru64(d, label, "#");
#endif
	while (ds->s.h & 0xF000000L)	{ /* assume high order issue */
		/*
		 * since the high order bits are not really being carried forward
		 * lets "assume" that the highorder bit of p should be raised by one
		 * and subtract again
		 */
		ps->s.h++;
		d = p - q;
#ifdef	XVERBOSE
	fprintf(stdout, "corrected\n", label, p, ps->s.h, ps->s.b);
	fprintf(stdout, "%s:ps:dbl:%016llX high:%08lX low:%08lx %012llu\n", label, p, ps->s.h, ps->s.b, p);
	fprintf(stdout, "%s:pq:dbl:%016llX high:%08lX low:%08lx %012llu\n", label, q, qs->s.h, qs->s.b, q);
	fprintf(stdout, "%s:di:dbl:%016llX high:%08lX low:%08lx %012llu ##\n", label, d, ds->s.h, ds->s.b, d);
#endif
#ifdef USEALARM
		pru64(d, label, "##");
#endif
/*
 * since we have an error, regardless, to keep output clear set to 0
 */
		d = (u_longlong_t) 0;
	}
	return(d);
}

static void perfstat_tot_diff(perfstat_netinterface_total_t *delta,
	perfstat_netinterface_total_t *p, perfstat_netinterface_total_t *q)
{
	/*
	 * since AIX 5.3 has a problem with printing values that are > 32-bit (libc)
	 * the easy solution is to compute the delta, and print based on the delta
	 * rather than using raw values
	 */
#ifdef TEST
	sub64(p->ibytes , q->ibytes, "ibytes");
#endif
	delta->ierrors = p->ierrors - q->ierrors;
	delta->oerrors = p->oerrors - q->oerrors;
#ifdef NORMAL
	delta->ibytes = p->ibytes - q->ibytes;
	delta->obytes = p->obytes - q->obytes;

	delta->ipackets = p->ipackets - q->ipackets;
	delta->opackets = p->opackets - q->opackets;
#else
	delta->ibytes   = sub64(p->ibytes, q->ibytes,     "ib");
	delta->obytes   = sub64(p->obytes, q->obytes,     "ob");

	delta->ipackets = sub64(p->ipackets, q->ipackets, "ip");
	delta->opackets = sub64(p->opackets, q->opackets, "op");
#ifdef XXXX
	delta->ierrors  = sub64(p->ierrors, q->ierrors,   "ie");
	delta->oerrors  = sub64(p->oerrors, q->oerrors,   "oe");
#endif
#endif
}

static void perfstat_id_diff(perfstat_netinterface_t *delta, perfstat_netinterface_t *p, perfstat_netinterface_t *q)
{
	/*
	 * since AIX 5.3 has a problem with printing values that are > 32-bit (libc)
** actually, the bigger issue is that the perfstat does not increase the high-long word as the counters increase **
	 * the easy solution is to compute the delta, and print based on the delta
	 * rather than using raw values
	 */
	delta->ipackets = p->ipackets - q->ipackets;
	delta->ibytes = p->ibytes - q->ibytes;
	delta->ierrors = p->ierrors - q->ierrors;
	delta->opackets = p->opackets - q->opackets;
	delta->obytes = p->obytes - q->obytes;
	delta->oerrors = p->oerrors - q->oerrors;
}

void perfstat_report(unsigned verbose)
{
    perfstat_id_t first;
    perfstat_netinterface_t delta;
    perfstat_netinterface_total_t delta_tot;
    perfstat_netinterface_total_t *pTotal2 = &delta_tot;
    int ret, i;
    unsigned long bytes, ibytes, obytes;
    double time_real();
    double  realt = time_real();
    double mb,imb,omb;

    perfstat_stop();
    perfstat_tot_diff(pTotal2, pTotal1, pTotal0);
    if (verbose < 2)
	return;
    header_metrics(verbose>1);

#ifdef XDEBUG
	ibytes = (pTotal1->ibytes - pTotal0->ibytes) + 0L;
	obytes = (pTotal1->obytes - pTotal0->obytes) + 0L;
	bytes = (pTotal1->ibytes - pTotal0->ibytes) +
		(pTotal1->obytes - pTotal0->obytes);
	 imb = (ibytes/realt)/(double) (1024*1024);
	 omb = (obytes/realt)/(double) (1024*1024);
	 mb = (bytes/realt)/(double) (1024*1024);
	 imb = (ibytes)/(double) (1024*1024);
	 omb = (obytes/realt)/(double) (1024*1024);
	 mb = (bytes/realt)/(double) (1024*1024);
#ifdef XXDEBUG
	fprintf(stdout, "imb omb mb :: %10.2f %10.2f %10.2f \n", imb, omb, mb);
	fflush(stdout);
#endif
	fprintf(stdout, "%8s mb:%12.2f ib:%12llu ip:%12llu ob:%12llu op:%12llu op:%12llu opsz:%12llu\n",
		"pTotal2", mb,
		(pTotal2->ibytes ),
		(pTotal2->ipackets),
		(pTotal2->ibytes )/(pTotal2->ipackets ),
		(pTotal2->obytes ),
		(pTotal2->opackets ),
		(pTotal2->obytes )/(pTotal2->opackets ));

	fprintf(stdout, "%8s mb:%12.2f ib:%12llu ip:%12llu ob:%12llu op:%12llu op:%12llu opsz:%12llu\n",
		"stack", mb,
		(pTotal1->ibytes - pTotal0->ibytes),
		(pTotal1->ipackets - pTotal0->ipackets),
		(pTotal1->ibytes - pTotal0->ibytes)/(pTotal1->ipackets - pTotal0->ipackets),
		(pTotal1->obytes - pTotal0->obytes),
		(pTotal1->opackets - pTotal0->opackets),
		(pTotal1->obytes - pTotal0->obytes)/(pTotal1->opackets - pTotal0->opackets));
#endif

    strcpy(first.name , FIRST_NETINTERFACE);
    ret = perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), tot);

    /* print statistics for each of the interfaces */
#ifdef NOBUG
    for (i = 0; i < ret; i++) {
		bytes =  (statp[i].ibytes - statq[i].ibytes) +
			 (statp[i].obytes - statq[i].obytes);
		 mb = (bytes/realt)/(double) (1024*1024);
		fprintf(stdout, "%8.2f %10llu %10llu %10llu %10llu %6llu %6llu %6s\n",
		mb,
		(statp[i].ibytes - statq[i].ibytes) / (1 * 1024),
		(statp[i].obytes - statq[i].obytes) / (1 * 1024),
		(statp[i].ipackets - statq[i].ipackets),
		(statp[i].opackets - statq[i].opackets),
		((statp[i].ipackets - statq[i].ipackets) > 0) ?
			(statp[i].ibytes - statq[i].ibytes)/(statp[i].ipackets - statq[i].ipackets) : 0,
		((statp[i].opackets - statq[i].opackets) > 0) ?
			(statp[i].obytes - statq[i].obytes)/(statp[i].opackets - statq[i].opackets) : 0,
		statp[i].name);
    }
#else
    for (i = 0; i < ret; i++) {
		bytes =  (sub64 (statp[i].ibytes,statq[i].ibytes,"ib") + sub64(statp[i].obytes,statq[i].obytes,"ob"));
		 mb = (bytes/realt)/(double) (1024*1024);
		fprintf(stdout, "%8.2f %10llu %10llu %10llu %10llu %6llu %6llu %6s\n",
		mb,
		sub64(statp[i].ibytes,statq[i].ibytes,NULL) / (1 * 1024),
		sub64(statp[i].obytes,statq[i].obytes,NULL) / (1 * 1024),
		sub64(statp[i].ipackets,statq[i].ipackets,NULL),
		sub64(statp[i].opackets,statq[i].opackets,NULL),
		(sub64(statp[i].ipackets,statq[i].ipackets,NULL) > 0) ?
			sub64(statp[i].ibytes,statq[i].ibytes,NULL)/sub64(statp[i].ipackets,statq[i].ipackets,NULL) : 0,
		(sub64(statp[i].opackets,statq[i].opackets,NULL) > 0) ?
			sub64(statp[i].obytes,statq[i].obytes,NULL)/sub64(statp[i].opackets,statq[i].opackets,NULL) : 0,
		statp[i].name);
    }
#endif
}
#endif
