/*
 *	A T T C P  -- M A I N
 *
 * Test TCP connection.  Makes a connection on port PORT, RORTREAD, PORTSEND (32765, 32766, 32767)
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2016: Michael Felt and AIXTOOLS.NET

 * $Date: 2017-03-24 09:46:19 +0000 (Fri, 24 Mar 2017) $
 * $Revision: 230 $
 * $Author: michael $
 * $Id: attcp_main.c 230 2017-03-24 09:46:19Z michael $
 */

#include <config.h>
#include <string.h>
#include "attcp.h"

/*
 * all routines will use the same option settings
 */
static	attcp_opt_p      ap_opts;
static	boolean	x_flag;
static	boolean udp;

void
sigintr( int x)
{
	/*
	attcp_report();
	attcp_rpt(TRUE,fmt,nbytes);
	*/
	fprintf(stderr, "attcp says: should be writing a report, but not ready\n");
}

void
sigpipe( int x)
{
	/*	int y = x; / * NULL operation to clear syntax warning */
}

void 
usage(void)
{
	fprintf(stdout,Usage);
	exit(1);
}

void
err(s)
char *s;
{
	syslog(LOG_ERR, "err:%s %s: errno:%d", x_flag?"-t":"-r", s, errno);
	fprintf(stderr,"err:attcp%s: ", x_flag?"-t":"-r");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	fflush(stdout);
	fflush(stderr);
	exit(1);
}

void
mes(s)
char *s;
{
	syslog(LOG_INFO, "%s %s", x_flag?"-t":"-r", s);
	fprintf(stdout,"mes(attcp%s: %s)\n", x_flag?"-t":"-r", s);
	fflush(stdout);
}

attcp_opt_p attcp_getopt(int argc,char **argv)
{
	unsigned long addr_tmp;
	int opt;
	char m;
	static attcp_opt_t      A;
	attcp_opt_p	a_opts = &A;
	/*
	 * first reset the old defaults in the new structure
	 */
	A.buflen	= 8 * 1024;
	A.nbuf		= 20 * 1024;
	A.bufalign	= 16 * 1024;
	A.port		= PORT;
	A.threads	= 1;
	A.verbose	= 0;
	A.fmt		= 'M';
	/*	A.sinkmode	= TRUE; */
	A.whereTo	= NULL;		/* will accept -c with -r to specify hostname */

	/* with -t is a NULL op, actually, but is accepted for specifying
	 * hostname (will add -h as extra alias!
	 */
	A.maxtime	= 10;
	A.maxtime	= 0;

	/*
	 * all other values default to zero or NULL
	 */

	if (argc < 2)
		usage();
	/*
	 * for now, expect to delete
	 */
	a_opts->argc = argc;
	a_opts->argv = argv;

	while ((opt = getopt(argc, argv, ":bdqruvxBDTc:f:h:i:l:n:p:s:t:")) != -1) {
		if (a_opts->verbose > 4) {
			fprintf(stderr, "opt:%c optarg:%s\n", opt, optarg);
			fprintf(stderr, "optind:%d argv[optind]:%s\n", optind, argv[optind]);
		}
		switch (opt) {

		case 'x':
			a_opts->x_flag = 1;
			break;
		case 'r':
			a_opts->x_flag = 0;
			break;
		case 'D':
			a_opts->options |= SO_DEBUG;
			break;
		case 'd':
			#ifdef TCP_NODELAY
			a_opts->nodelay = 1;
			#else
			fprintf(stdout, 
			    "ttcp: -D option ignored: TCP_NODELAY socket option not supported\n");
			#endif
			break;
		case 'n':
			a_opts->nbuf = atoi(optarg);
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				a_opts->nbuf *= 1024;
			else if (m == 'm')
				a_opts->nbuf *= 1024 * 1024;

			break;
		case 'l':
			a_opts->buflen = atoi(optarg);

			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'm')
				a_opts->buflen *= 1024 * 1024;
			else if (m == 'k')
				a_opts->buflen *= 1024;

			break;
		case 't':
			a_opts->threads = atoi(optarg);
			break;
		case 'p':
			a_opts->port = atoi(optarg);
			break;
		case 'u':
			a_opts->udp = 1;
			break;
		case 's':
			#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
			a_opts->sockbufsize = atoi(optarg);
			if (a_opts->sockbufsize <= 0) {
				char msg[256];
				sprintf(msg,"-s %s: bad socketsize: a_opts->socketbufsize == %d",optarg,a_opts->sockbufsize);
				err(msg);
			}
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				a_opts->sockbufsize *= 1024;
			else if (m == 'm')
				a_opts->sockbufsize *= 1024 * 1024;
			#else
			fprintf(stdout, 
			    "ttcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
			#endif
			break;
		case 'f':
			a_opts->fmt = *optarg;
			break;
		case 'T':
			a_opts->touchdata = 1;
			break;
		case 'c':
			a_opts->c_flag = 1;
		case 'h':
			a_opts->whereTo = optarg;
			break;
		case 'q':
			a_opts->q_flag = !a_opts->q_flag;
			break;
		case 'v':
			a_opts->verbose++;
			break;
		case 'i':
			a_opts->maxtime = atoi(optarg);
			break;
		case 'B':
			fprintf(stdout, 
			    "attcp: -B option ignored: needs to be re-implemented\n");
			break;

		default:
			usage();
		}
	}
	return(a_opts);
}

/*
 * create a socket
 */
int
attcp_socket(attcp_conn_p c, attcp_opt_p a_opts)
{
	static socket_cnt = 0;
	char message[128];
	int addr_family = AF_INET;
	int sd = -1;
	char *whereTo = NULL;
	unsigned long addr_tmp = 0L;

	struct sockaddr_in *sinHere  = &c->sinHere;
	struct sockaddr_in *sinThere = &c->sinThere;

	if(a_opts->x_flag || a_opts->c_flag)  { /* xmit */
		/* make connection to a host, either collect or as xmitter */
		if ((optind == a_opts->argc) && (a_opts->whereTo == NULL))
			usage();

		bzero((char *)sinThere, sizeof(*sinThere));
		bzero((char *)sinHere, sizeof(*sinThere));
		if (a_opts->whereTo == NULL)
			whereTo = a_opts->whereTo = a_opts->argv[optind];
		else
			whereTo = a_opts->whereTo;
		/* prepare msg for just in case ! */
		sprintf(message,"bad hostname:%s", whereTo);

		if (atoi(whereTo) > 0 )  {
			/* Numeric */
			sinThere->sin_family = addr_family;
			sinThere->sin_addr.s_addr = inet_addr(whereTo);
			if (sinThere->sin_addr.s_addr == INADDR_NONE)
				err(message);
		} else {
			struct hostent	*addr;
			if ((addr=gethostbyname(whereTo)) == NULL)
				err(message);
			sinThere->sin_family = addr->h_addrtype;
			bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
			sinThere->sin_addr.s_addr = addr_tmp;
		}
		sinThere->sin_port = htons(a_opts->port);
		sinHere->sin_port = 0;		/* free choice */
	} else { /* rcvr */
		sinHere->sin_port =  htons(a_opts->port);
	}

	if ((sd = socket(addr_family, a_opts->udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
		err("socket");
	if (bind(sd, SA(sinHere), sizeof(*sinHere)) < 0)
		err("bind");
	c->sd = sd;
	socket_cnt++;
	/*
	 * syslog statement about socket definition
	 * move to own function!
	 */
	{
		struct sockaddr_in sock;
		socklen_t socklen = sizeof(sock);
		mes("peer sock names");

		if (getsockname(sd,  SA(&sock), &socklen) < 0) {
			err("getsockname");
		}
		if (a_opts->verbose) {
			fprintf(stderr,"attcp: bind() on %s:%d\n", inet_ntoa(sock.sin_addr), ntohs(sock.sin_port));
		}
		mes("syslog bind()");
		syslog(LOG_INFO, "bind() on %s", inet_ntoa(sock.sin_addr));
	}
	return(sd);
}

attcp_setoption( attcp_conn_p c, attcp_opt_p a_opts)
{
	int sd = c->sd;
	int one = 1;
	#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
	if (a_opts->sockbufsize) {
		fprintf(stderr, "\nsetsockopt sockbufsize: %0x\n", a_opts->sockbufsize);
		fprintf(stderr, "\nsetsockopt sockbufsize: x_flag:%d\n", a_opts->x_flag);
		if (a_opts->x_flag) {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_SNDBUF,
			    &a_opts->sockbufsize, sizeof(a_opts->sockbufsize)) < 0)
				err("setsockopt: sndbuf");
			mes("sndbuf");
		} else {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_RCVBUF,
			    &a_opts->sockbufsize, sizeof(a_opts->sockbufsize)) < 0)
				err("setsockopt: rcvbuf");
			mes("rcvbuf");
		}
	}
	#endif
	/* options = 0 || SO_DEBUG */
	if (a_opts->options)  {
		if( setsockopt(sd,
		    SOL_SOCKET, a_opts->options,
		    &one, sizeof(one)) < 0)
			err("setsockopt");
	}
	#ifdef TCP_NODELAY
	if (a_opts->nodelay) {
		struct protoent *p;
		p = getprotobyname("tcp");
		if( p && setsockopt(sd, p->p_proto, TCP_NODELAY, 
		    &one, sizeof(one)) < 0)
			err("setsockopt: nodelay");
		mes("nodelay");
	}
	#endif
}

attcp_syslog1(attcp_opt_p a_opts)
{
	#define SEVERITY LOG_INFO
	int     allow_severity = SEVERITY;      /* run-time adjustable */
	int     deny_severity = LOG_WARNING;    /* ditto */
	char msg1[128];
	char msg2[128];
	char msg3[128];
	/* Report request and invoke the real daemon program. */
	/*
     The message is identical to a printf(3) format string, except that `%m'
     is replaced by the current error message.  (As denoted by the global
     variable errno; see strerror(3).)  A trailing newline is added if none is
     present.
	*/
	/*
	 * looking for use of getnameinfo() for after connect!
	 */

	if (a_opts->maxtime > 0)
		sprintf(msg1, "buflen=%d, time=%d sec", a_opts->buflen, a_opts->maxtime);
	else
		sprintf(msg1, "buflen=%d, nbuf=%d, bytes=%uMB",
		    a_opts->buflen, a_opts->nbuf, (a_opts->buflen * a_opts->nbuf) / (1024 * 1024));

	*msg2 = 0; /* null terminate, just in case */
	if (a_opts->sockbufsize)
		sprintf(msg2, ", sockbufsize=%d", a_opts->sockbufsize);

	sprintf(msg3, ", %s%s%s.%d",
	    a_opts->udp    ? "udp" : "tcp",
	    a_opts->x_flag ? "=>" : "<=",
	    a_opts->whereTo,
	    a_opts->port);
	syslog(allow_severity, "%s%s%s", msg1, msg2, msg3);
}

attcp_conn( attcp_conn_p c, attcp_opt_p a_opts)
{
	register char *buf;
	int	sd = c->sd;
	register int nbuf = a_opts->nbuf;
	register int buflen = a_opts->buflen;
	struct	sockaddr_in	*sinThere = &c->sinThere;

	if (a_opts->udp && a_opts->buflen < 5) {
		a_opts->buflen = 5;		/* send more than the sentinel size */
	}

	if ( (buf = (char *)malloc(a_opts->buflen+a_opts->bufalign)) == (char *)NULL)
		err("malloc");
	c->buf = buf;
	/*
	 * probably pointless, leaving for historical purpose
	 */
	if (a_opts->bufalign != 0)
		buf +=(a_opts->bufalign - ((int)buf % a_opts->bufalign) + a_opts->bufoffset) % a_opts->bufalign;

	if (!a_opts->udp)  {			/* TCP connection */
		signal(SIGINT, sigintr);
		signal(SIGPIPE, sigpipe);
		if (a_opts->x_flag || a_opts->c_flag) {
			/* We are the client if transmitting */
			if(connect(sd, SA(sinThere), sizeof(*sinThere) ) < 0)
				err("connect");
			/* if (!a_opts->q_flag) mes("connect"); */
		} else {			/* UDP connection */
			struct sockaddr_in	*frominet;
			socklen_t	fromlen;
			frominet = &c->frominet;

			/* otherwise, we are the server and 
			 * should listen for the connections
			 */
			if (!a_opts->q_flag)
				mes("listen");
			listen(sd,0);   /* allow a queue of 0 */
			/* options = 0 || SO_DEBUG */
			if(a_opts->options)  {
				int one = 1;
				if( setsockopt(sd, SOL_SOCKET, a_opts->options, &one, sizeof(one)) < 0)
					err("setsockopt");
			}
			fromlen = sizeof(*frominet);
			if((sd=accept(sd, SA(frominet), &fromlen) ) < 0)
				err("accept");
			if (!a_opts->q_flag)
				mes("accept");
		}
		{
			struct sockaddr_in peer,sock;
			socklen_t peerlen = sizeof(peer);
			socklen_t socklen = sizeof(peer);
			char *fmt = "attcp: accept() %s:%d with %s:%d\n";

			if (getpeername(sd,  SA(&peer), &peerlen) < 0) {
				err("getpeername");
			}
			if (getsockname(sd,  SA(&sock), &socklen) < 0) {
				err("getsockname");
			}
			
			if (a_opts->verbose) {
				fprintf(stderr, fmt,
					inet_ntoa(sock.sin_addr), ntohs(sock.sin_port),
					inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
			}
			syslog(LOG_INFO, fmt, inet_ntoa(sock.sin_addr), ntohs(sock.sin_port),
				inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
		}
	}
}
/*
 * break point for work stuff
 * ABOVE - get connected
 * BELOW - xfer bytes
 */
attcp_xfer( attcp_conn_p c, attcp_opt_p a_opts)
{
	register void	*buf = c->buf;
	int	sd = c->sd;
	register int	nbuf = a_opts->nbuf;
	register int	buflen = a_opts->buflen;
	register uint64_t	*nbytes = &c->nbytes;
	register int	cnt;

	timer0();
	errno = 0;

	if (a_opts->x_flag)  { /* client mode */
		pattern( buf, a_opts->buflen );
		if(a_opts->udp)  (void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr start */
		if (0 == a_opts->maxtime) {
			while (nbuf-- && Nwrite(sd,buf,buflen, c, a_opts) == buflen)
				*nbytes += buflen;
		} else {
			while (Nwrite(sd,buf,a_opts->buflen,c,a_opts) == a_opts->buflen) {
				*nbytes += a_opts->buflen;
				if (elapsed_time() > a_opts->maxtime) {
					break;
				}
			}
		}
		if(a_opts->udp)  (void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr end */
	} else { /* server mode - listening */
		if (a_opts->udp) {
			while ((cnt=Nread(sd, buf, buflen, c, a_opts)) > 0)  {
				static int going = 0;
				if( cnt <= 4 )  {
					if( going )
						break;	/* "EOF" */
					going = 1;
					/*
					 * restart timer0 when udp!
					 */
					timer0();
				} else {
					*nbytes += cnt;
				}
			}
		} else {
			int total_bytes = a_opts->nbuf * buflen;
			while ((cnt=Nread(sd,buf,buflen, c, a_opts)) > 0)  {
				/*
				fprintf(stderr,"xfer:Nread\n");
				*/
				*nbytes += cnt;
				if (a_opts->c_flag && (a_opts->maxtime == 0 ) ) {
					total_bytes -= cnt;
					if (total_bytes <= 0) {
						break;
					}
				}
				if (a_opts->maxtime != 0) {
					if (elapsed_time() > a_opts->maxtime) {
						break;
					}
				}
			}
		}
	}
	if(errno) err("IO XXX");
	timer1();
	if(a_opts->udp&&a_opts->x_flag)  {
		(void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a_opts); /* rcvr end */
	}
}

pattern( cp, cnt )
register char *cp;
register int cnt;
{
	register char c;
	c = 0;
	while( cnt-- > 0 )  {
		while( !isprint((c&0x7F)) )  c++;
		*cp++ = (c++&0x7F);
	}
}

/*
 *			N R E A D
 */
Nread( int sd, void *buf, unsigned count, attcp_conn_p c, attcp_opt_p a_opts)
{
	register ulong *sockCalls = &c->sockcalls;
	struct sockaddr_in from;
	socklen_t len = sizeof(from);
	register int cnt;
	#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
	#endif
	if( a_opts->udp )  {
		cnt = recvfrom( sd, buf, count, 0, SA(&from), &len );
		*sockCalls += 1;
	} else {
		if( a_opts->B_flag )
			cnt = mread( sd, buf, count, c );	/* fill buf */
		else {
again2:			
			cnt = read( sd, buf, count );
			if( cnt<0 && errno == EINTR )  {
				errno = 0;
				goto again2;
			}
			*sockCalls += 1;
		}
		if (a_opts->touchdata && cnt > 0) {
			register int i = cnt, sum;
			register char *b = buf;
			while (i--)
				sum += *b++;
		}
	}
	return(cnt);
}

/*
 *			N W R I T E
 */
Nwrite(int sd, void *buf, unsigned count, attcp_conn_p c, attcp_opt_p a_opts)
{
	register int cnt;
	register ulong *sockCalls = &c->sockcalls;
	struct	sockaddr_in *sinThere;

	#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
	#endif
	if( a_opts->udp )  {
		sinThere = &c->sinThere;
again:
		cnt = sendto( sd, buf, count, 0, SA(sinThere), sizeof(*sinThere) );
		*sockCalls += 1;
		if( cnt<0 && errno == ENOBUFS )  {
			delay(18000);
			errno = 0;
			goto again;
		}
	} else {
again2:		
		cnt = write( sd, buf, count );
		if( cnt<0 && errno == EINTR )  {
			errno = 0;
			goto again2;
		}
		*sockCalls += 1;
	}
	return(cnt);
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
mread(sd, bufp, n, c)
int sd;
void	*bufp;
unsigned	n;
attcp_conn_p     c;
{
	register unsigned	count = 0;
	register int		nread;
	register ulong *sockCalls = &c->sockcalls;

	do {
		nread = read(sd, bufp, n-count);
		*sockCalls += 1;
		if(nread < 0)  {
			perror("ttcp_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	} while(count < n);

	return((int)count);
}

char stats[128];
unsigned long nbytes;           /* bytes on net */
unsigned long sockCalls;        /* # of socket() I/O calls */
double cput, realt;             /* user, real time (seconds) */

main(int argc, char **argv)
{
	/*
	 * static for now, need to come from malloc() in the future
	 */
	static attcp_conn_t     C;
	char *syslog_idstr;
	char *cpt;

	attcp_opt_p      ax;
	attcp_conn_p     c = &C;
	int		sd; /* socket descriptor */

	/*
	 * set the static variable
	 */
	ax = ap_opts = attcp_getopt(argc,argv);

	x_flag = ax->x_flag;	/* for message routines to set -t or -r */
	udp = (ax->udp != 0);	/* for various routines, mainly read/write */

	#define FACILITY LOG_UUCP
	#define LOG_OPTION (LOG_PID|LOG_NDELAY|LOG_CONS)
	cpt = strrchr(argv[0], '/');
	syslog_idstr = (cpt) ? cpt+1 : argv[0];

	openlog(syslog_idstr, LOG_OPTION, FACILITY);
	/* _r routines, I am not ready yet
	openlog_r(syslog_idstr, LOG_OPTION, FACILITY);
	 */

	/*
	 * atm - mono-threaded - static Connection structure
	 */
	sd = attcp_socket(&C, ap_opts);
	if (C.sd != sd) {
		fprintf(stderr, "socket creation failed\n");
		exit(-1);
	}
	if (ax->verbose > 3)
		fprintf(stderr,"finished attcp_socket()\n");

	attcp_syslog1(ap_opts);
	if (ax->verbose > 3)
		fprintf(stderr,"finished attcp_syslog1()\n");

	attcp_setoption( &C, ap_opts);
	if (ax->verbose > 3)
		fprintf(stderr,"finished attcp_setoption()\n");

	fflush(stdout);
	fflush(stderr);

	attcp_conn( &C, ap_opts);
	if (ax->verbose > 3)
		fprintf(stderr,"finished attcp_conn()\n");

	attcp_xfer( &C, ap_opts);
	if (ax->verbose > 3)
		fprintf(stderr,"finished attcp_xfer()\n");
	sockCalls += c->sockcalls;

	attcp_rpt(ax->verbose,ax->fmt,c->nbytes);
	exit(0);
}
