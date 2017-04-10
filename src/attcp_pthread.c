/*
 *	A T T C P 
 * pthread management
 *
 * Test TCP connection.  Makes a connection on port 32765, 32766, 32767
 * and transfers fabricated buffers or data copied from stdin.
 *
		host = argv[optind];
 *      A T T C P
 *
 * Test TCP connection.  Makes a connection on port 49149,49150,49151
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2016: Michael Felt and AIXTOOLS.NET
 *

 * $Date: 2016-01-17 19:09:44 +0000 (Sun, 17 Jan 2016) $
 * $Revision: 179 $
 * $Author: michael $
 * $Id: attcp_pthread.c 179 2016-01-17 19:09:44Z michael $
 */
#include <config.h>

#include "attcp.h"
init(int role, char host[], (struct sockaddr_in *) Here[],There[]);
{
	(struct sockaddr_in) *sHere = Here;
	(struct sockaddr_in) *sThere = There;
	*sHere = (struct sockaddr_in *) malloc(sizeof(sockaddr_in));
	*sThere = (struct sockaddr_in *) malloc(sizeof(sockaddr_in));

	bzero((char *)sThere, sizeof(sockaddr_in));
	bzero((char *)sHere, sizeof(sockaddr_in));

	if (role == TRANSMIT)
	{
		if (atoi(host) > 0 )  {
			/* Numeric */
			sThere->sin_family = AF_INET;
			sThere->sin_addr.s_addr = inet_addr(host);
		} else {
			if ((addr=gethostbyname(host)) == NULL)
				err("bad hostname");
			sThere->sin_family = addr->h_addrtype;
			bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
			sThere->sin_addr.s_addr = addr_tmp;
		}
		sThere->sin_port = htons(port);
		sHere->sin_port = 0;		/* free choice */
	} else {
		/* rcvr */
		sHere->sin_port =  htons(port);
	}

	if ((sd = socket(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
		err("socket");
	if (bind(sd, SA(&sHere), sizeof(sockaddr_in)) < 0)
		err("bind");

	if (sockbufsize) {
	    if (trans) {
		if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
		    sizeof sockbufsize) < 0)
			err("setsockopt: sndbuf");
		mes("sndbuf");
	    } else {
		if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
		    sizeof sockbufsize) < 0)
			err("setsockopt: rcvbuf");
		mes("rcvbuf");
	    }
	}

	if (!udp)  {
	    signal(SIGPIPE, sigpipe);
	    if (trans || c_flag) {
		/* We are the client if transmitting */
		if (options)  {
			if( setsockopt(sd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
				err("setsockopt");
		}
		if (nodelay) {
			struct protoent *p;
			p = getprotobyname("tcp");
			if( p && setsockopt(sd, p->p_proto, TCP_NODELAY, 
			    &one, sizeof(one)) < 0)
				err("setsockopt: nodelay");
			mes("nodelay");
		}
		if(connect(sd, SA(&sThere), sizeof(sockaddr_in) ) < 0)
			err("connect");
		if (!q_flag) mes("connect");
	    } else {
		/* otherwise, we are the server and 
	         * should listen for the connections
	         */
		listen(sd,0);   /* allow a queue of 0 */
		if(options)  {
			if( setsockopt(sd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
				err("setsockopt");
		}
		fromlen = sizeof(sockaddr_in);
		domain = AF_INET;
		if((sd=accept(sd, SA(&frominet), &fromlen) ) < 0)
			err("accept");
		{ struct sockaddr_in peer;
		  socklen_t peerlen = sizeof(sockaddr_in);
		  if (getpeername(sd,  SA(&peer), 
				&peerlen) < 0) {
			err("getpeername");
		  }
		  if (!q_flag) fprintf(stderr,"attcp-r: accept from %s\n", 
			inet_ntoa(peer.sin_addr));
		}
	    }
	}
	prep_timer();
	errno = 0;
	if (sinkmode) {      
		register int cnt;
		if (trans)  {
			pattern( buf, buflen );
			if(udp)  (void)Nwrite( sd, buf, 4 ); /* rcvr start */
			if (0 == interval) {
				while (nbuf-- && Nwrite(sd,buf,buflen) == buflen)
					nbytes += buflen;
			} else {
				while (Nwrite(sd,buf,buflen) == buflen) {
					nbytes += buflen;
					if (elapsed_time() > interval) {
						break;
					}
				}
			}
			if(udp)  (void)Nwrite( sd, buf, 4 ); /* rcvr end */
		} else {
			if (udp) {
			    while ((cnt=Nread(sd,buf,buflen)) > 0)  {
				    static int going = 0;
				    if( cnt <= 4 )  {
					    if( going )
						    break;	/* "EOF" */
					    going = 1;
					    prep_timer();
				    } else {
					    nbytes += cnt;
				    }
			    }
			} else {
				int total_bytes = nbuf * buflen;
			    while ((cnt=Nread(sd,buf,buflen)) > 0)  {
				    nbytes += cnt;
					if (c_flag && (0 == interval) ) {
						total_bytes -= cnt;
						if (total_bytes <= 0) {
							break;
						}
					}
					if (0 != interval) {
						if (elapsed_time() > interval) {
							break;
						}
					}
			    }
			}
		}
	} else {
		register int cnt;
		if (trans)  {
			while((cnt=read(0,buf,buflen)) > 0 &&
			    Nwrite(sd,buf,cnt) == cnt)
				nbytes += cnt;
		}  else  {
			while((cnt=Nread(sd,buf,buflen)) > 0 &&
			    write(1,buf,cnt) == cnt)
				nbytes += cnt;
		}
	}
	if(errno) err("IO");
	(void)read_timer(stats,sizeof(stats));
	if(udp&&trans)  {
		(void)Nwrite( sd, buf, 4 ); /* rcvr end */
		(void)Nwrite( sd, buf, 4 ); /* rcvr end */
		(void)Nwrite( sd, buf, 4 ); /* rcvr end */
		(void)Nwrite( sd, buf, 4 ); /* rcvr end */
	}
	if( cput <= 0.0 )  cput = 0.001;
	if( realt <= 0.0 )  realt = 0.001;
	if (q_flag) {
		fprintf(stdout,
		"%s\n",
		outfmt(nbytes/realt));
	} else
	{		
		fprintf(stdout,
		"attcp%s: %.0f bytes in %.2f real seconds = %s/sec +++\n",
		trans?"-t":"-r",
		nbytes, realt, outfmt(nbytes/realt));
	if (verbose) {
	    fprintf(stdout,
		"attcp%s: %.0f bytes in %.2f CPU seconds = %s/cpu sec\n",
		trans?"-t":"-r",
		nbytes, cput, outfmt(nbytes/cput));
	}
	fprintf(stdout,
		"attcp%s: %d socket() I/O calls, msec/call = %.2f, calls/sec = %.2f\n",
		trans?"-t":"-r",
		sockCalls,
		1024.0 * realt/((double)sockCalls),
		((double)sockCalls)/realt);
	fprintf(stdout,"attcp%s: %s\n", trans?"-t":"-r", stats);
	if (verbose) {
	    fprintf(stdout,
		"attcp%s: buffer address %#x\n",
		trans?"-t":"-r",
		buf);
	}
	} /* !q_flag */
	exit(0);

usage:
	fprintf(stderr,Usage);
	exit(1);
}

void
err(s)
char *s;
{
	fprintf(stderr,"attcp%s: ", trans?"-t":"-r");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	exit(1);
}

void
mes(s)
char *s;
{
	fprintf(stderr,"attcp%s: %s\n", trans?"-t":"-r", s);
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

char *
outfmt(b)
double b;
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

static struct	timeval time0;	/* Time at which timing started */
static struct	rusage ru0;	/* Resource utilization at the start */

static void prusage();
static void tvadd();
static void tvsub();
static void psecs();

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
/*
 * initial libperfstat interface
 */
	do_initialization();
	start_metrics();

	gettimeofday(&time0, (struct timezone *)0);
	getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 * 
 */
double
read_timer(str,len)
char *str;
{
	struct timeval timedol;
	struct rusage ru1;
	struct timeval td;
	struct timeval tend, tstart;
	char line[132];

	getrusage(RUSAGE_SELF, &ru1);
	gettimeofday(&timedol, (struct timezone *)0);
/*
 * insert for libperfstat texts
 */
	finish_metrics();
	fprintf(stdout,"\n");
	prusage(&ru0, &ru1, &timedol, &time0, line);
	(void)strncpy( str, line, len );

	/* Get real time */
	tvsub( &td, &timedol, &time0 );
	realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

	/* Get CPU time (user+sys) */
	tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
	tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
	tvsub( &td, &tend, &tstart );
	cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
	if( cput < 0.00001 )  cput = 0.00001;
	return( cput );
}

double
elapsed_time()
{
	struct timeval timedol;
	struct timeval td;
	double et = 0.0;

	gettimeofday(&timedol, (struct timezone *)0);
	tvsub( &td, &timedol, &time0 );
	et = td.tv_sec + ((double)td.tv_usec) / 1000000;

	return( et );
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

static void
tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
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

/*
 *			N R E A D
 */
Nread( sd, buf, count )
int sd;
void *buf;
int count;
{
	struct sockaddr_in from;
	socklen_t len = sizeof(from);
	register int cnt;
	if( udp )  {
		cnt = recvfrom( sd, buf, count, 0, SA(&from), &len );
		sockCalls++;
	} else {
		if( b_flag )
			cnt = mread( sd, buf, count );	/* fill buf */
		else {
			cnt = read( sd, buf, count );
			sockCalls++;
		}
		if (touchdata && cnt > 0) {
			register int c = cnt, sum;
			register char *b = buf;
			while (c--)
				sum += *b++;
		}
	}
	return(cnt);
}

/*
 *			N W R I T E
 */
Nwrite( sd, buf, count )
int sd;
void *buf;
int count;
{
	register int cnt;
	if( udp )  {
again:
		cnt = sendto( sd, buf, count, 0, SA(&sThere), sizeof(sockaddr_in) );
		sockCalls++;
		if( cnt<0 && errno == ENOBUFS )  {
			delay(18000);
			errno = 0;
			goto again;
		}
	} else {
		cnt = write( sd, buf, count );
		sockCalls++;
	}
	return(cnt);
}

void
delay(us)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = us;
	(void)select( 1, (fd_set*)0, (fd_set *)0, (fd_set *)0, &tv );
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
mread(sd, bufp, n)
int sd;
register char	*bufp;
unsigned	n;
{
	register unsigned	count = 0;
	register int		nread;

	do {
		nread = read(sd, bufp, n-count);
		sockCalls++;
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
