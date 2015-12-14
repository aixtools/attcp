/*
 *	A T T C P  -- M A I N
 *
 * Test TCP connection.  Makes a connection on port PORT, RORTREAD, PORTSEND
 * and transfers fabricated buffers or data copied from stdin.
 *

 * $Date:$
 * $Revision:$
 * $Author:$
 * $Id:$
 *
 */

#include <config.h>
#include "attcp.h"
/*
static attcp_opt_p      ax;
*/
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
	syslog(LOG_ERR, "attcp%s %s: %m", x_flag?"-t":"-r", s);

	fprintf(stderr,"attcp%s: ", x_flag?"-t":"-r");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	exit(1);
}

void
mes(s)
char *s;
{
/*	syslog(LOG_INFO, "%s %s", x_flag?"-t":"-r", s); */
	syslog(LOG_INFO, "%s", s);
	fprintf(stdout,"attcp%s: %s\n", x_flag?"-t":"-r", s);
}

attcp_options(int argc,char **argv, attcp_opt_p a)
{
	unsigned long addr_tmp;
	int opt;
	char m;
	
	if (argc < 2)
		usage();
/*
 * for now, expect to delete
 */
	a->argc = argc;
	a->argv = argv;

	while ((opt = getopt(argc, argv, ":bdqruvxBDTc:f:h:i:l:n:p:s:t:")) != -1) {
		if (a->verbose > 3) {
			fprintf(stderr, "opt:%c optarg:%s\n", opt, optarg);
			fprintf(stderr, "optind:%d argv[optind]:%s\n", optind, argv[optind]);
		}
		switch (opt) {

		case 'x':
			a->x_flag = 1;
			break;
		case 'r':
			a->x_flag = 0;
			break;
		case 'D':
			a->options |= SO_DEBUG;
			break;
		case 'd':
#ifdef TCP_NODELAY
			a->nodelay = 1;
#else
			fprintf(stdout, 
	"ttcp: -D option ignored: TCP_NODELAY socket option not supported\n");
#endif
			break;
		case 'n':
			a->nbuf = atoi(optarg);
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				a->nbuf *= 1024;
			else if (m == 'm')
				a->nbuf *= 1024 * 1024;
			
			break;
		case 'l':
			a->buflen = atoi(optarg);
			
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'm')
				a->buflen *= 1024 * 1024;
			else if (m == 'k')
				a->buflen *= 1024;
			
			break;
		case 't':
			a->threads = atoi(optarg);
			break;
		case 'p':
			a->port = atoi(optarg);
			break;
		case 'u':
			a->udp = 1;
			break;
		case 's':
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
			a->sockbufsize = atoi(optarg);
			if (a->sockbufsize <= 0) {
				char msg[256];
				sprintf(msg,"-s %s: bad socketsize: a->socketbufsize == %d",optarg,a->sockbufsize);
				err(msg);
			}
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				a->sockbufsize *= 1024;
			else if (m == 'm')
				a->sockbufsize *= 1024 * 1024;
#else
			fprintf(stdout, 
"ttcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
#endif
			break;
		case 'f':
			a->fmt = *optarg;
			break;
		case 'T':
			a->touchdata = 1;
			break;
		case 'c':
			a->c_flag = 1;
		case 'h':
			a->whereTo = optarg;
			break;
		case 'q':
			a->q_flag = !a->q_flag;
			break;
		case 'v':
			a->verbose++;
			break;
		case 'i':
			a->maxtime = atoi(optarg);
			break;
		case 'B':
			fprintf(stdout, 
	"attcp: -B option ignored: needs to be re-implemented\n");
			break;

		default:
			usage();
		}
	}
}

int
attcp_socket(attcp_conn_p c, attcp_opt_p a)
{
	char message[128];
	int addr_family = AF_INET;
	int sd = -1;
	char *whereTo = NULL;
        unsigned long addr_tmp = 0L;

	struct sockaddr_in *sinHere  = &c->sinHere;
	struct sockaddr_in *sinThere = &c->sinThere;
	struct hostent	*addr;

	if(a->x_flag || a->c_flag)  {
		/* make connection to a host, either collect or as xmitter */
		if ((optind == a->argc) && (a->whereTo == NULL))
			usage();

		bzero((char *)sinThere, sizeof(*sinThere));
		bzero((char *)sinHere, sizeof(*sinThere));
		if (a->whereTo == NULL)
			whereTo = a->whereTo = a->argv[optind];
		else
			whereTo = a->whereTo;
		/* prepare msg for just in case ! */
		sprintf(message,"bad hostname:%s", whereTo);

		if (atoi(whereTo) > 0 )  {
			/* Numeric */
			sinThere->sin_family = addr_family;
			sinThere->sin_addr.s_addr = inet_addr(whereTo);
			if (sinThere->sin_addr.s_addr == INADDR_NONE)
				err(message);
		} else {
			if ((addr=gethostbyname(whereTo)) == NULL)
				err(message);
			sinThere->sin_family = addr->h_addrtype;
			bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
			sinThere->sin_addr.s_addr = addr_tmp;
		}
		sinThere->sin_port = htons(a->port);
		sinHere->sin_port = 0;		/* free choice */
	} else {
		/* rcvr */
		sinHere->sin_port =  htons(a->port);
	}

	if ((sd = socket(addr_family, a->udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
		err("socket");
	if (bind(sd, SA(sinHere), sizeof(*sinHere)) < 0)
		err("bind");
	c->sd = sd;
	return(sd);
}

attcp_setoption( attcp_conn_p c, attcp_opt_p a)
{
	int sd = c->sd;
	int one = 1;
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
	if (a->sockbufsize) {
fprintf(stderr, "\nsetsockopt sockbufsize: %0x\n", a->sockbufsize);
fprintf(stderr, "\nsetsockopt sockbufsize: x_flag:%d\n", a->x_flag);
	    if (a->x_flag) {
		if (setsockopt(sd,
		    SOL_SOCKET, SO_SNDBUF,
		    &a->sockbufsize, sizeof(a->sockbufsize)) < 0)
			err("setsockopt: sndbuf");
		mes("sndbuf");
	    } else {
		if (setsockopt(sd,
		    SOL_SOCKET, SO_RCVBUF,
		    &a->sockbufsize, sizeof(a->sockbufsize)) < 0)
			err("setsockopt: rcvbuf");
		mes("rcvbuf");
	    }
	}
#endif
/* options = 0 || SO_DEBUG */
		if (a->options)  {
			if( setsockopt(sd,
			    SOL_SOCKET, a->options,
			    &one, sizeof(one)) < 0)
				err("setsockopt");
		}
#ifdef TCP_NODELAY
		if (a->nodelay) {
			struct protoent *p;
			p = getprotobyname("tcp");
			if( p && setsockopt(sd, p->p_proto, TCP_NODELAY, 
			    &one, sizeof(one)) < 0)
				err("setsockopt: nodelay");
			mes("nodelay");
		}
#endif
}

attcp_syslog1(attcp_opt_p a)
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

    if (a->maxtime > 0)
	sprintf(msg1, "buflen=%d, time=%d sec", a->buflen, a->maxtime);
    else
        sprintf(msg1, "buflen=%d, nbuf=%d, bytes=%uMB",
	    a->buflen, a->nbuf, (a->buflen * a->nbuf) / (1024 * 1024));
    *msg2 = 0; /* null terminate, just in case */
    if (a->sockbufsize)
	sprintf(msg2, ", sockbufsize=%d", a->sockbufsize);
    syslog(allow_severity, "%s%s", msg1, msg2);
    sprintf(msg3, "%s %s%s%s.%d",
	(a->x_flag)?"transmit to":"receive from",
	a->udp?"udp":"tcp",
	(a->x_flag) ? "=>" : "<=",  a->whereTo,a->port);
    syslog(allow_severity, "%s", msg3);
}

attcp_prep( attcp_conn_p c, attcp_opt_p a)
{
	int one = 1;
	register char *buf;
	int	sd = c->sd;
	register int nbuf = a->nbuf;
	register int buflen = a->buflen;
	struct	sockaddr_in	*sinThere = &c->sinThere;

	if (a->udp && a->buflen < 5) {
	    a->buflen = 5;		/* send more than the sentinel size */
	}

	if ( (buf = (char *)malloc(a->buflen+a->bufalign)) == (char *)NULL)
		err("malloc");
	c->buf = buf;
/*
 * probably pointless, leaving for historical purpose
 */
	if (a->bufalign != 0)
		buf +=(a->bufalign - ((int)buf % a->bufalign) + a->bufoffset) % a->bufalign;

	if (!a->udp)  {
	    signal(SIGINT, sigintr);
	    signal(SIGPIPE, sigpipe);
	    if (a->x_flag || a->c_flag) {
		/* We are the client if transmitting */
		if(connect(sd, SA(sinThere), sizeof(*sinThere) ) < 0)
			err("connect");
		if (!a->q_flag) mes("connect");
	    } else {
		struct sockaddr_in	*frominet;
		socklen_t	fromlen;
		frominet = &c->frominet;

		/* otherwise, we are the server and 
	         * should listen for the connections
	         */
		listen(sd,0);   /* allow a queue of 0 */
/* options = 0 || SO_DEBUG */
		if(a->options)  {
			if( setsockopt(sd,
			    SOL_SOCKET, a->options,
			    &one, sizeof(one)) < 0)
				err("setsockopt");
		}
		fromlen = sizeof(*frominet);
		if((sd=accept(sd, SA(frominet), &fromlen) ) < 0)
			err("accept");
		{
		  struct sockaddr_in peer;
		  socklen_t peerlen = sizeof(peer);
		  if (getpeername(sd,  SA(&peer), &peerlen) < 0) {
			err("getpeername");
		  }
		  if (a->verbose)
			fprintf(stdout,"attcp: accept() from %s\n",
			    inet_ntoa(peer.sin_addr));
		  syslog(LOG_INFO, "accept() from %s", inet_ntoa(peer.sin_addr));
		}
	    }
	}
}
/*
 * break point for work stuff
 * ABOVE - get connected
 * BELOW - xfer bytes
 */
attcp_xfer( attcp_conn_p c, attcp_opt_p a)
{
	register void	*buf = c->buf;
		 int	sd = c->sd;
	register int	nbuf = a->nbuf;
	register int	buflen = a->buflen;
	register uint64_t	*nbytes = &c->nbytes;
	register int	cnt;

	timer0();
	errno = 0;

	if (a->x_flag)  { /* client mode */
		pattern( buf, a->buflen );
		if(a->udp)  (void)Nwrite( sd, buf, 4, c, a ); /* rcvr start */
		if (0 == a->maxtime) {
			while (nbuf-- && Nwrite(sd,buf,buflen, c, a) == buflen)
				*nbytes += buflen;
		} else {
			while (Nwrite(sd,buf,a->buflen,c,a) == a->buflen) {
				*nbytes += a->buflen;
				if (elapsed_time() > a->maxtime) {
					break;
				}
			}
		}
		if(a->udp)  (void)Nwrite( sd, buf, 4, c, a ); /* rcvr end */
	} else { /* server mode - listening */
		if (a->udp) {
		    while ((cnt=Nread(sd, buf, buflen, c, a)) > 0)  {
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
			int total_bytes = a->nbuf * buflen;
		    while ((cnt=Nread(sd,buf,buflen, c, a)) > 0)  {
/*
fprintf(stderr,"xfer:Nread\n");
*/
			    *nbytes += cnt;
				if (a->c_flag && (a->maxtime == 0 ) ) {
					total_bytes -= cnt;
					if (total_bytes <= 0) {
						break;
					}
				}
				if (a->maxtime != 0) {
					if (elapsed_time() > a->maxtime) {
						break;
					}
				}
		    }
		}
	}
	if(errno) err("IO XXX");
	timer1();
	if(a->udp&&a->x_flag)  {
		(void)Nwrite( sd, buf, 4, c, a ); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a ); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a ); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c, a ); /* rcvr end */
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
Nread( sd, buf, count, c, a )
int sd;
void *buf;
unsigned count;
attcp_conn_p     c;
attcp_opt_p     a;
{
	register ulong *sockCalls = &c->sockcalls;
	struct sockaddr_in from;
	socklen_t len = sizeof(from);
	register int cnt;
#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
#endif
	if( a->udp )  {
		cnt = recvfrom( sd, buf, count, 0, SA(&from), &len );
		*sockCalls += 1;
	} else {
		if( a->B_flag )
			cnt = mread( sd, buf, count, c );	/* fill buf */
		else {
again2:			cnt = read( sd, buf, count );
			if( cnt<0 && errno == EINTR )  {
				errno = 0;
				goto again2;
			}
			*sockCalls += 1;
		}
		if (a->touchdata && cnt > 0) {
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
Nwrite( sd, buf, count, c, a )
int sd;
void *buf;
unsigned count;
attcp_conn_p     c;
attcp_opt_p     a;
{
	register int cnt;
	register ulong *sockCalls = &c->sockcalls;
	struct	sockaddr_in *sinThere;

#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
#endif
	if( a->udp )  {
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
again2:		cnt = write( sd, buf, count );
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

#define FACILITY LOG_LOCAL2

char stats[128];
unsigned long nbytes;           /* bytes on net */
unsigned long sockCalls;        /* # of socket() I/O calls */
double cput, realt;             /* user, real time (seconds) */

main(int argc, char **argv)
{
/*
 * static for now, need to come from malloc() in the future
 */
	static attcp_opt_t      A;
	static attcp_conn_t     C;

	attcp_conn_p     c = &C;
	attcp_opt_p      a = &A;
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

	/*
	 * set the static variable
	ax = &A;
	 */
	attcp_options(argc,argv,&A);
	x_flag = a->x_flag;	/* for message routines to set -t or -r */
	udp = (a->udp != 0);	/* for various routines, mainly read/write */

    openlog(argv[0], LOG_PID|LOG_ODELAY, FACILITY);
/* _r routines, not ready yet
    (void) openlog_r(argv[0], LOG_PID|LOG_ODELAY, FACILITY);
 */
	
	attcp_socket(&C, &A);
if (A.verbose > 3)
	fprintf(stderr,"finished attcp_socket()\n");

	attcp_syslog1(&A);
if (A.verbose > 3)
	fprintf(stderr,"finished attcp_syslog1()\n");

	attcp_setoption( &C, &A);
if (A.verbose > 3)
	fprintf(stderr,"finished attcp_setoption()\n");

fflush(stdout);
fflush(stderr);

	attcp_prep( &C, &A);
if (A.verbose > 3)
	fprintf(stderr,"finished attcp_prep()\n");

	attcp_xfer( &C, &A);
if (A.verbose > 3)
	fprintf(stderr,"finished attcp_xfer()\n");
	sockCalls += c->sockcalls;

	attcp_rpt(a->verbose,a->fmt,c->nbytes);
	exit(0);
}
