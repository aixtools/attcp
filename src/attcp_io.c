/*
 *	A T T C P  -- I O
 *
 * Test TCP connection.  Makes a connection on port PORT, PORTREAD, PORTSEND (32765, 32766, 32767)
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
