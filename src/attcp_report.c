/*
 *      A T T C P
 *	R E P O R T - report results
 *
 * Test TCP connection.  Makes a connection on port 32765, 32766, 32767
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2016: Michael Felt and AIXTOOLS.NET
 *
 * $Date: 2017-04-12 18:37:47 +0000 (Wed, 12 Apr 2017) $
 * $Revision: 239 $
 * $Author: michael $
 * $Id: attcp_report.c 239 2017-04-12 18:37:47Z michael $
 */

#include "attcp.h"
#include <config.h>

#include <sys/resource.h>
typedef	int	boolean;

extern char stats[128];

double
nvalue(double b, char fmt)
{
    switch (fmt) {
/*
 * calculate Bytes per sec - formated to 'fmt'  from the RAW Bytes per sec value - b
 */
	case 'G':
	    return(b / (1024.0 * 1024.0 * 1024.0));
	    break;
	default:
	case 'M':
	    return(b / (1024.0 * 1024.0));
	    break;
	case 'K':
	    return(b / 1024.0);
	    break;
	case 'B':
	case 'R':
	    return(b );
	    break;
/*
 * approximate bps from the Bytes per sec value - b
 */
	case 'g':
	    return(b / (0.99 * 102.40 * 1024.0 * 1024.0));
	    break;
	case 'm':
	    return(b / (0.99 * 102.40 * 1024.0));
	    break;
	case 'k':
	    return(b / (0.99 * 102.40));
	    break;
	case 'b':
	case 'r':
	    return(b / 0.99 );
	    break;
    }
}

char *
nformat(char fmt)
{
    static char obuf[50];
    switch (fmt) {
/*
 * calculate Bytes per sec - formated to 'fmt'  from the RAW Bytes per sec value - b
 */
	case 'G':
	    return("GB");
	    break;
	default:
	case 'M':
	    return("MB");
	    break;
	case 'K':
	    return("KB");
	    break;
	case 'B':
	    return("B");
	    break;
	case 'R':
	    return("");
	    break;
/*
 * approximate bps from the Bytes per sec value - b
 */
	case 'g':
	    return("Gbit");
	    break;
	case 'm':
	    return("Mbit");
	    break;
	case 'k':
	    return("Kbit");
	    break;
	case 'b':
	    return("bit");
	    break;
	case 'r':
	    return("");
	    break;
    }
}

char *
outfmt(double b, char fmt)
{
    static char obuf[50];
    switch (fmt) {
/*
 * calculate Bytes per sec - formated to 'fmt'  from the RAW Bytes per sec value - b
 */
	case 'G':
	    sprintf(obuf, "%.2f GB", b / (1024.0 * 1024.0 * 1024.0));
	    break;
	default:
	case 'M':
	    sprintf(obuf, "%.2f MB", b / (1024.0 * 1024.0));
	    break;
	case 'K':
	    sprintf(obuf, "%.2f KB", b / 1024.0);
	    break;
	case 'B':
	    sprintf(obuf, "%.2f B", b );
	    break;
	case 'R':
	    sprintf(obuf, "%.2f", b );
	    break;
/*
 * approximate bps from the Bytes per sec value - b
 */
	case 'g':
	    sprintf(obuf, "%.2f Gbit", b / (0.99 * 102.40 * 1024.0 * 1024.0));
	    break;
	case 'm':
	    sprintf(obuf, "%.2f Mbit", b / (0.99 * 102.40 * 1024.0));
	    break;
	case 'k':
	    sprintf(obuf, "%.2f Kbit", b / (0.99 * 102.40));
	    break;
	case 'b':
	    sprintf(obuf, "%.2f bit", b / 0.99 );
	    break;
	case 'r':
	    sprintf(obuf, "%.2f", b / 0.99 );
	    break;
    }
    return obuf;
}

#define END(x)	{while(*x) x++;}

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

#include <syslog.h>
#define SEVERITY LOG_INFO

void
attcp_rpt(boolean verbose, char fmt, uint64_t nbytes)
{
	double physc, realtime;		/* user, real time (seconds) */
	double time_busy(), time_real();
	int	allow_severity = SEVERITY;

	realtime = time_real();
	physc = time_busy();
	double bsec = (nbytes/realtime);
	double mbsec = bsec / (double) (1024*1024);
	double fmbsec = nvalue(bsec, fmt);
	char *bformat = nformat(fmt);

#ifdef HAVE_LIBPERFSTAT
	perfstat_report(verbose);
#endif

	if( physc <= 0.0 )  physc = 0.001;
        if( realtime <= 0.0 )  realtime = 0.001;

	if (verbose < 1)
#ifdef XXX
		fprintf(stdout, "%s/sec\n",  outfmt((nbytes/realtime),fmt));
#endif
		fprintf(stdout, "%.2f %s/sec\n", fmbsec, bformat);
	else
	{
          fprintf(stdout,"ATTCP Summary\n");

          fprintf(stdout,"%8s %10s %8s %8s %6s %10s %8s %7s %9s\n",
		"MB/Sec", "MByte", "seconds", "physc", "%busy", "Calls", "B/call", "ms/call", "call/sec");
          fprintf(stdout,"%8s %10s %8s %8s %6s %10s %8s %7s %9s\n",
		"========", "========", "=======", "=======", "=====",
		"=========","======","=======","========");
          fprintf(stdout, "%8.2f %10llu %8.2f %8.2f %6.1f %10llu %8llu %7.2f %9.1f\n",
		mbsec, nbytes/(1024*1024), realtime, physc, (100*physc)/realtime,
		sockCalls, nbytes / sockCalls,
		(realtime)/((double)sockCalls)*1024.0, ((double)sockCalls)/realtime);
	}
	if (verbose > 2)
          syslog(SEVERITY,"%8s %10s %8s %8s %6s %10s %8s %7s %9s\n",
		"MB/Sec", "MByte", "seconds", "physc", "%busy", "Calls", "B/call", "ms/call", "call/sec");

        syslog(SEVERITY, "%8.2f %10llu %8.2f %6.1f %10llu %8llu %7.2f %9.1f\n",
		mbsec, nbytes/(1024*1024), realtime, (100*physc)/realtime,
		sockCalls, nbytes / sockCalls,
		(realtime)/((double)sockCalls)*1024.0, ((double)sockCalls)/realtime);
}
