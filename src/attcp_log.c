/*
 *	A T T C P  -- L O G
 *
 * Test TCP connection.  Makes a connection on port PORT, PORTREAD, PORTSEND (32765, 32766, 32767)
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2017: Michael Felt and AIXTOOLS.NET

 * $Date: 2017-04-11 16:59:04 +0000 (Tue, 11 Apr 2017) $
 * $Revision: 233 $
 * $Author: michael $
 * $Id: attcp_main.c 233 2017-04-11 16:59:04Z michael $
 */

#include <config.h>
#include <string.h>
#include "attcp.h"

#define FACILITY LOG_UUCP
#define SEVERITY LOG_INFO
#define LOG_OPTION (LOG_PID|LOG_NDELAY|LOG_CONS)

attcp_log_opts(attcp_opt_p a_opts)
{
	int     allow_severity = SEVERITY;      /* run-time adjustable */
	int     deny_severity = LOG_WARNING;    /* ditto */
	char msg1[128];
	char msg2[128];
	char msg3[128];
	/* Report connection options */
	sprintf(msg1, "%s%s%s.%d",
	    a_opts->udp    ? "udp" : "tcp",
	    a_opts->x_flag ? "=>" : "<=",
	    a_opts->peername,
	    a_opts->port);
	/*
	 * looking for use of getnameinfo() for after connect!
	 */

	if (a_opts->maxtime > 0)
		sprintf(msg2, "TIMED: buflen=%d, time=%d sec",
		    a_opts->buflen, a_opts->maxtime);
	else
		sprintf(msg2, "SIZED: buflen=%d, bytes=%uMB, nbuf=%d",
		    a_opts->buflen, (a_opts->buflen * a_opts->nbuf) / (1024 * 1024), a_opts->nbuf);

	*msg3 = 0; /* null terminate, just in case */
	if (a_opts->sockbufsize)
		sprintf(msg3, ", sockbufsize=%d", a_opts->sockbufsize);

	syslog(LOG_INFO, "%s %s%s", msg1, msg2, msg3);
}

attcp_log_init(char *prgname)
{
	char *syslog_idstr;
	char *cpt;


	cpt = strrchr(prgname, '/');
	syslog_idstr = (cpt) ? cpt+1 : prgname;

	openlog(syslog_idstr, LOG_OPTION, FACILITY);
	/* _r routines, I am not ready yet
	openlog_r(syslog_idstr, LOG_OPTION, FACILITY);
	 */
}


/*
 * print the endpoints of a socket
 */
void
attcp_log_name(int sd, char *buf, int verbose)
{
	struct sockaddr_in peer,sock;
	socklen_t peerlen = sizeof(peer);
	socklen_t socklen = sizeof(peer);
	char *fmt = "sd:%d name: %s:%d peer %s:%d\n";

	bzero((char *)&sock, sizeof(sock));
	bzero((char *)&peer, sizeof(peer));

	if (getpeername(sd,  SA(&peer), &peerlen) < 0) {
		err("getpeername");
	}
	if (getsockname(sd,  SA(&sock), &socklen) < 0) {
		err("getsockname");
	}

	sprintf(buf, fmt, sd,
		inet_ntoa(sock.sin_addr), ntohs(sock.sin_port),
		inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
	if (verbose)
		syslog(LOG_DEBUG, buf);
}
