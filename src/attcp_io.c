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

attcp_xmit( attcp_conn_p c)
{
	void		*buf = c->buf;
	int		sd = c->sd;
	attcp_set_p	cs = &c->settings;
	int		io_cnt = cs->io_count;
	uint64_t	io_size = cs->io_size;
	uint64_t	nbytes = 0;
	uint64_t	maxbytes = cs->maxbytes;
	int		maxtime = cs->maxtime;
	int		nread = 55;

#ifdef VERBOSE
	uint64_t	*io_sz;
	io_sz = &cs->io_size;
	fprintf(stderr,"XMIT 0: C:%08X tid %ld sd:%d %s ", cs, c->tid, c->sd,
			cs->transport == SOCK_DGRAM ? "udp" : "tcp");
	fprintf(stderr,"time:%d nbytes:%d io_sz:%d maxbytes:%d\n",
			cs->maxtime, c->nbytes, cs->io_size, cs->maxbytes);
	fprintf(stderr,"XMIT 0: C:%08X tid %ld sd:%d %s ", cs, c->tid, c->sd,
			cs->transport == SOCK_DGRAM ? "udp" : "tcp");
	fprintf(stderr,"time:%d nbytes:%d io_sz:%d maxbytes:%d\n",
			cs->maxtime, c->nbytes, cs->io_size, cs->maxbytes);
	fprintf(stderr,"XMIT 1: C:%08X tid %ld sd:%d ", cs, c->tid, c->sd);
	fprintf(stderr,"%s time:%d nbytes:%ld ",
			cs->transport == SOCK_DGRAM ? "udp" : "tcp",
			cs->maxtime, c->nbytes);
	fprintf(stderr,"io_sz:%X cs->io_sz:%d ",
			&cs->io_size, cs->io_size);
	fprintf(stderr,"io_sz:%X *io_sz:%ld ",
			io_sz, *io_sz);
	fprintf(stderr,"io_sz:%X cs->io_sz:%d\n",
			&io_size, io_size);
	fflush(stderr);
#endif

	pattern( buf, io_size );
	if(cs->transport == SOCK_DGRAM)
		(void)Nwrite( sd, buf, 4, c); /* wakeup rcvr */
	if (maxtime <= 0) { /* use number of buffers instead */
		do {
			nread = Nwrite(sd,buf,io_size,c);
			if (nread < 0)
				break;
			nbytes += nread;
		} while (io_cnt-- && (nbytes <= maxbytes) && !c->io_done);
	} else {
#ifdef DEBUG
		fprintf(stderr,"C:%08X start time loop ", c);
		fprintf(stderr,"tid %ld sd:%d ", c->tid, c->sd);
		fprintf(stderr,"etime:%d nbytes:%lld maxbytes:%lld\n",
			(int) elapsed_time(), nbytes, maxbytes);
		fprintf(stderr,"c:%08X sd:%d buf:%08X len:%ld\n",
			c, sd, buf, io_size);
		fflush(stderr);
#endif
		do {
			nread = Nwrite(sd,buf,io_size,c);
			if (nread <= 0)	{
				mes("xmit break");
				break;
			}
			nbytes = nbytes + nread;
		} while (!c->io_done && ((int) elapsed_time() < maxtime) && (nbytes <= maxbytes));
	}
	if(cs->transport == SOCK_DGRAM)
		Nwrite( sd, buf, 4, c); /* rcvr end */
	c->io_done++;
	c->nbytes = nbytes;
#ifdef VERBOSE
	fprintf(stderr,"XX: %lld %lld %lld\n", nbytes, maxbytes, nbytes-maxbytes);
	fprintf(stderr,"XMIT 2: C:%08X tid %ld sd:%d ", cs, c->tid, c->sd);
	fprintf(stderr,"%s time:%d nbytes:%d io_sz:%d maxbytes:%d\n",
			cs->transport == SOCK_DGRAM ? "udp" : "tcp",
			cs->maxtime, c->nbytes, cs->io_size, cs->maxbytes);
	fprintf(stderr,"XMIT 4: C:%08X tid %ld sd:%d %s ", cs, c->tid, c->sd,
			cs->transport == SOCK_DGRAM ? "udp" : "tcp");
	fprintf(stderr,"time:%d nbytes:%d io_sz:%d maxbytes:%d\n",
			cs->maxtime, c->nbytes, cs->io_size, cs->maxbytes);
#ifdef XXX
	fprintf(stderr,"%s time:%d nbytes:%d io_sz:%d\n",
			cs->transport == SOCK_DGRAM ? "udp" : "tcp",
			cs->maxtime, c->nbytes, cs->io_size);
#endif
	fprintf(stderr,"XMIT 3: C:%08X tid %ld sd:%d ", cs, c->tid, c->sd);
	fprintf(stderr,"etime:%d nbytes:%lld maxbytes:%lld\n",
			(int) elapsed_time(), nbytes, maxbytes);
	fflush(stderr);
#endif
}
attcp_rcvr(attcp_conn_p c)
{
	void		*buf = c->buf;
	int		sd = c->sd;
	attcp_set_p	cs = &c->settings;
	int		io_cnt = cs->io_count;
	int		io_size = cs->io_size;
	uint64_t	maxbytes = cs->maxbytes;
	int		maxtime = cs->maxtime;
	uint64_t	nbytes = 0;
	int		nread;
	int		going = 0;

	if (cs->transport == SOCK_DGRAM) {
		while ((nread=Nread(sd, buf, io_size, c)) > 0)  {
			if( nread <= 4 )  {
				/*
				 * restart timer0 when udp!
				 */
				if( going++ == 0 )
					timer0(RUSAGE_THREAD, c);
				else
					break;	/* "EOF" */
			} else {
				nbytes += nread;
			}
		}
	} else if (maxtime <= 0) { /* use number of buffers instead */
                do {
                        nread = Nread(sd,buf,io_size,c);
                        if (nread < 0)
                                break;
                        nbytes += nread;
                } while (io_cnt-- && (nbytes <= maxbytes));
        } else {
#ifdef DEBUG
		ulong looper = 0;
		fprintf(stderr,"C:%08X start time loop ", c);
		fprintf(stderr,"tid %ld sd:%d ", c->tid, c->sd);
		fprintf(stderr,"etime:%d nbytes:%lld maxbytes:%lld\n",
			(int) elapsed_time(), nbytes, maxbytes);
		fprintf(stderr,"c:%08X sd:%d buf:%08X len:%ld\n",
			c, sd, buf, io_size);
		fflush(stderr);
#endif
                do {
                        nread = Nread(sd,buf,io_size,c);
                        if (nread < 0)
                                break;

#ifdef DEBUG
			looper++;
			if (nbytes == 0)
				fprintf(stderr,"attcp_rcvr: nread: %d bytes\n", nread);
			else if ((looper % 100000) == 0)
				fprintf(stderr,"\ncio:%d et:%04.2f nb:%lld",
					c->io_done, elapsed_time(), nbytes - maxbytes);
			else if ((looper % 10000) == 0)
				fputc('.', stderr);
#endif
                        nbytes += nread;
                } while (!c->io_done && (elapsed_time() < maxtime) && (nbytes <= maxbytes));
        }
	c->io_done++;
	c->nbytes = nbytes;
}
attcp_xfer( attcp_conn_p c)
{
	timer0(RUSAGE_THREAD, c);
	errno = 0;

#ifdef DEBUG
	fprintf(stderr,"xfer 1: C:%08X tid %ld sd:%d\n", c, c->tid, c->sd);
#endif
	if (c->settings.x_flag)  { /* client send mode */
		attcp_xmit(c);
	} else { /* server mode - listening */
		attcp_rcvr(c);
	}
	timer1(RUSAGE_THREAD, c);
	/* if XMIT and UDP try to ensure client knows to stop */
	if ((c->settings.transport == SOCK_DGRAM) && c->settings.x_flag)  {
		int sd = c->sd;
		char buf[4];
		(void)Nwrite( sd, buf, 4, c); /* signal rcvr end */
		(void)Nwrite( sd, buf, 4, c); /* signal rcvr end */
		(void)Nwrite( sd, buf, 4, c); /* rcvr end */
		(void)Nwrite( sd, buf, 4, c); /* rcvr end */
	}
#ifdef DEBUG
	fprintf(stderr,"xfer 2: C:%08X tid %ld sd:%d\n", c, c->tid, c->sd);
#endif
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
Nread( int sd, void *buf, unsigned count, attcp_conn_p c)
{
	ulong *sockCalls = &c->sockcalls;
	struct	sockaddr_in *sinPeer;
	socklen_t	peerlen = sizeof(*sinPeer);
	ssize_t cnt;

#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
#endif
	if( c->settings.transport == SOCK_DGRAM )  {
		sinPeer = &c->sinPeer;
		cnt = recvfrom( sd, buf, count, 0, SA(sinPeer), &peerlen );
		*sockCalls += 1;
	} else {
		if( c->settings.B_flag )
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
		if (c->settings.touchdata && cnt > 0) {
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
Nwrite(int sd, void *buf, unsigned count, attcp_conn_p c)
{
	register int cnt;
	register ulong *sockCalls = &c->sockcalls;
	struct	sockaddr_in *sinPeer;

	#ifdef USEALARM
	if (my_alarm)
		perfstat_alarm();
	#endif
	if( c->settings.transport == SOCK_DGRAM )  {
		sinPeer = &c->sinPeer;
again:
		cnt = sendto( sd, buf, count, 0, SA(sinPeer), sizeof(*sinPeer) );
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
