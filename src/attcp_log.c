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

/*
 * Report connection options
 */
attcp_log_opts(attcp_opt_p a_opts)
{
	int	allow_severity	= SEVERITY;      /* run-time adjustable */
	int	deny_severity	= LOG_WARNING;    /* ditto */
	attcp_set_p
		cs		= &a_opts->settings;
	char	msg1[128];
	char	msg2[128];
	char	msg3[128];
	sprintf(msg1, "%s%s%s.%d",
	    cs->transport == SOCK_DGRAM ? "udp" : "tcp",
	    cs->x_flag ? "=>" : "<=",
	    a_opts->peername,
	    a_opts->port);
	/*
	 * looking for use of getnameinfo() for after connect!
	 */

	/*
	 * expected bytes: min(maxBytes,io_count * io_size);
	 * WORRY about format later!
	 */
	if (cs->maxtime > 0)
		sprintf(msg2, "TIMED: max:%0.2f, buflen=%d, time=%d sec",
		    cs->io_size, cs->maxtime);
	else
		sprintf(msg2, "SIZED: buflen=%d, bytes=%uMB, nbuf=%d",
		    cs->io_size, (cs->io_size * cs->io_count) / (1024 * 1024), cs->io_size);

	*msg3 = 0; /* null terminate, just in case */
	if (cs->sockbufsize)
		sprintf(msg3, ", sockbufsize=%d", cs->sockbufsize);

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
attcp_log_socket(int sd, char *buf, int verbose)
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
