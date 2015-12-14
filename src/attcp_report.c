/*
 *	A T T C P . C
 *
 * Test TCP connection.  Makes a connection on port 55000
 * and transfers fabricated buffers or data copied from stdin.
 *
 *	R E P O R T - report results
 */

#include <config.h>
#include "attcp.h"

#include <sys/resource.h>
typedef	int	boolean;

extern char stats[128];
extern unsigned long sockCalls;	/* # of socket() I/O calls */

char *
outfmt(double b, char fmt)
{
    static char obuf[50];
    switch (fmt) {
	case 'G':
	    sprintf(obuf, "%.2f GB", b / 1024.0 / 1024.0 / 1024.0);
	    break;
	default:
	case 'K':
	    sprintf(obuf, "%.2f KB", b / 1024.0);
	    break;
	case 'M':
	    sprintf(obuf, "%.2f MB", b / 1024.0 / 1024.0);
	    break;
	case 'B':
	    sprintf(obuf, "%.2f B", b );
	    break;
	case 'R':
	    sprintf(obuf, "%.2f", b );
	    break;
	case 'g':
	    sprintf(obuf, "%.2f Gbit", b * 8.0 / 1024.0 / 1024.0 / 1024.0);
	    break;
	case 'k':
	    sprintf(obuf, "%.2f Kbit", b * 8.0 / 1024.0);
	    break;
	case 'm':
	    sprintf(obuf, "%.2f Mbit", b * 8.0 / 1024.0 / 1024.0);
	    break;
	case 'b':
	    sprintf(obuf, "%.2f bit", b * 8.0 );
	    break;
	case 'r':
	    sprintf(obuf, "%.2f", b * 8);
	    break;
    }
    return obuf;
}

static void
psecs(l,cp)
long l;
register char *cp;
{
	register int i;

	i = l / 3600;
	if (i) {
		sprintf(cp,"%d:", i);
		END(cp);
		i = l % 3600;
		sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
		END(cp);
	} else {
		i = l;
		sprintf(cp,"%d", i / 60);
		END(cp);
	}
	i %= 60;
	*cp++ = ':';
	sprintf(cp,"%d%d", i / 10, i % 10);
}

static void
prusage(r0, r1, e, b, outp)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
	char *outp;
{
	struct timeval tdiff;
	register time_t t;
	register char *cp;
	register int i;
	int ms;

	t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}
	cp = "%u user %s sys %E real %P\n\t%Xi+%Dd %M maxrss\n\t%F+%R pf\n\t%C csw\n\t%r+%x messages";
	cp = "%u user %s sys %E real %P percent";
	cp = "  %u %s %E %P";
	for (; *cp; cp++)  {
		if (*cp != '%')
			*outp++ = *cp;
		else if (cp[1]) switch(*++cp) {

		case 'u': /* user */
			tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 's': /* system */
			tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'E': /* elasped */
			psecs(ms / 100, outp);
			END(outp);
			break;

		case 'P': /* percent */
			sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
			END(outp);
			break;

		case 'W': /* swaps */
			i = r1->ru_nswap - r0->ru_nswap;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'X': /* delta shared mem size - rss */
			sprintf(outp,"%d", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
			END(outp);
			break;

		case 'D': /* delta unshared mem size - rss data+stack */
			sprintf(outp,"%d", t == 0 ? 0 :
			    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'K': /* delta rss mem shared + unshared */
			sprintf(outp,"%d", t == 0 ? 0 :
			    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
			    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'M': /* maxrss */
			sprintf(outp,"%d", r1->ru_maxrss);
			END(outp);
			break;

		case 'F': /* page faults */
			sprintf(outp,"%d", r1->ru_majflt-r0->ru_majflt);
			END(outp);
			break;

		case 'R': /* page reclaims */
			sprintf(outp,"%d", r1->ru_minflt-r0->ru_minflt);
			END(outp);
			break;

		case 'I':
			sprintf(outp,"%d", r1->ru_inblock-r0->ru_inblock);
			END(outp);
			break;

		case 'O':
			sprintf(outp,"%d", r1->ru_oublock-r0->ru_oublock);
			END(outp);
			break;

		case 'x': /* messages out */
			i = r1->ru_msgsnd - r0->ru_msgsnd;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'r': /* messages received */
			i = r1->ru_msgrcv - r0->ru_msgrcv;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'C': /* voluntary+involuntarty context switches */
			sprintf(outp,"%d+%d", r1->ru_nvcsw-r0->ru_nvcsw,
				r1->ru_nivcsw-r0->ru_nivcsw );
			END(outp);
			break;
		}
	}
	*outp = '\0';
}

attcp_rpt(unsigned verbose, char fmt, uint64_t nbytes)
{
	double cput, realt;		/* user, real time (seconds) */
	double time_busy(), time_real();

	realt = time_real();
	cput = time_busy();

#ifdef HAVE_PERFSTAT
	perfstat_report(verbose);
#endif

	if( cput <= 0.0 )  cput = 0.001;
        if( realt <= 0.0 )  realt = 0.001;

        if (verbose < 1) {
                fprintf(stdout,
                "%s\n",
                outfmt((nbytes/realt),fmt));
        } else
        {
	  double mbsec = (nbytes/realt)/(double) (1024*1024);

          fprintf(stdout,"ATTCP Summary\n");

          fprintf(stdout,"%8s %10s %8s %6s %10s %8s %7s %9s\n",
		"MB/Sec", "MByte", "seconds", "%busy", "Calls", "B/call", "ms/call", "call/sec");
          fprintf(stdout,"%8s %10s %8s %6s %10s %8s %7s %9s\n",
		"========", "========", "=======", "=====",
		"=========","======","=======","========");
          fprintf(stdout, "%8.2f %10llu %8.2f %6.1f %10lu %8llu %7.2f %9.1f\n",
		mbsec, nbytes/(1024*1024), realt, (100*cput)/realt,
		sockCalls, nbytes / sockCalls,
		(realt)/((double)sockCalls)*1024.0, ((double)sockCalls)/realt);
	}

}
