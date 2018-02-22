/*
 *	A T T C P 
 *
 * Test TCP connection.  Makes a connection on port 32765,32766,32767
 * and transfers fabricated buffers or data copied from stdin.
 * Copyright 2013-2016: Michael Felt and AIXTOOLS.NET
 *

 * $Date: 2017-04-12 20:08:59 +0000 (Wed, 12 Apr 2017) $
 * $Revision: 243 $
 * $Author: michael $
 * $Id: attcp.h 243 2017-04-12 20:08:59Z michael $
 */
#ifndef _AIX
#define _GNU_SOURCE
#endif

#include <pthread.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>		/* struct timeval */

#include <sys/resource.h>

#include <syslog.h>

#define SA( p )               ( (struct sockaddr *) (p) )

/* port number 49151 is reserved by IANA */
/* 49152 -4 = 49148 */
#define EPHEMERAL_LOW	(1<<15)	/* 32768 */
#define PORT		(EPHEMERAL_LOW-1)
#define PORTREAD	(PORT-1)
#define PORTSEND	(PORT-2)

#define A_TCP SOCK_STREAM
#define A_UDP SOCK_DGRAM

typedef	unsigned long	ulong;
typedef	unsigned short	ushort;

typedef struct	attcp_settings {
	int	c_flag;		/* connect to as rcvr to peer */
	int	x_flag;		/* transmit aka xmit */
	int	q_flag;		/* quiet == !q_flag */
	int	verbosity;	/* increase level with -v, -vv, etc. */
	int	transport;	/* transport mode : true udp, false tcp */
	int	io_count;
	int	maxtime;
	uint64_t	io_size;
	uint64_t	maxbytes;
	ulong	sockbufsize;	/* overide default socket size (mbuf == max)*/
	int 	so_options;	/* SO_DEBUG */
	int	no_delay;	/* setsockoption */


	int	touchdata;
	int	B_flag;		/* full BLOCKS */
	int	role;		/* role */
}	attcp_set_t, *attcp_set_p;

typedef struct	attcp_opt {
/* order of the variables here is how the compiler complains.
 * Will make more orderly another time
 */
	int	threads;	/* mono or multi-threads */
	char	*peername;	/* hostname or IP address of peer */
	ushort	port;		/* socket port number */
	char	fmt;		/* -f argument, whatever that is ??? */
	attcp_set_t settings;
}	attcp_opt_t,	*attcp_opt_p;

typedef struct	attcp_conn {
	pthread_t	tid;
	int		io_done;
	int	sd;	/* socket descriptor */
	char	*buf;	/* buffer for I/O read/write */
	ulong	sockcalls;	/* track socket calls made - read or write */
	uint64_t	nbytes;		/* track connection bytes transferred */
	struct sockaddr_in
			sinHere,
			sinPeer;
	struct hostent	*addr;
	struct   rusage ru0;     /* Resource utilization at the start */
	struct   rusage ru1;     /* Resource utilization at the end */
	attcp_set_t	settings;
}	attcp_conn_t,	*attcp_conn_p;

/* remove as global - better is as passed variable */
#ifdef GLOBALS
extern attcp_opt_p	a;
extern attcp_conn_p	c;
#endif

void usage();
void err();
void mes();

void timer0(int Who, attcp_conn_p c);
void timer1(int Who, attcp_conn_p c);

void prep_timer();
double read_timer();
double elapsed_time();

int Nread(int sd, void*buf, unsigned count, attcp_conn_p c);
int Nwrite(int sd, void*buf, unsigned count, attcp_conn_p c);

void delay(int u_sec);

int mread(int sd, void*buf, unsigned count, attcp_conn_p     c);
char *outfmt(double b, char fmt);

typedef int boolean;

void attcp_rcvr( attcp_conn_p c);
void attcp_xmit( attcp_conn_p c);
void attcp_xfer( attcp_conn_p c);

void
attcp_rpt(boolean verbose, char fmt, uint64_t nbytes);

pthread_t
attcp_pthread_start(int threads);

boolean
attcp_pthread_init(int threads);

void
attcp_thread_stop(int threads);

uint64_t
attcp_thread_done(int threads);

/*
 * create the threads and set the options
 */
void
attcp_pthread_socket(attcp_opt_p a_opts);

void
attcp_socket(attcp_conn_p c, attcp_opt_p a);

/*
 * connect a socket
 */
void
attcp_connect( attcp_conn_p c);

/*
 * set options on a socket
 */
void
attcp_setoption( attcp_conn_p c);

/*
 * Report connection options
 */
void
attcp_log_opts(attcp_opt_p a_opts);
void
attcp_log_init(char *prgname);


extern char Usage[];
extern uint64_t	sockCalls;
