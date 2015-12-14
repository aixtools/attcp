/*
 *	A T T C P 
 *
 * Test TCP connection.  Makes a connection on port 55000,55001,55002
 * and transfers fabricated buffers or data copied from stdin.
 *

 * $Date:$
 * $Revision:$
 * $Author:$
 * $Id:$
 */


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

/* 49152 -3 = 49149 */
#define PORT		((1<<15|1<<14)-3)
#define PORTREAD	(PORT+1)
#define PORTSEND	(PORT+2)

typedef	unsigned long	ulong;
typedef	unsigned short	ushort;

typedef struct	attcp_opt {
/* order of the variables here is how the compiler complains.
 * Will make more orderly another time
 */
	int	B_flag;		/* full BLOCKS */
	int	c_flag;
	int	q_flag;		/* verbose == !q_flag */
	int	verbose;	/* talky talky -- debug level with -vvvv, or -v9, etc.. */
	int	x_flag;		/* transmit aka xmit */
	int	threads;	/* mono or multi-threads */
	int 	options;	/* SO_DEBUG */
	int	nodelay;	/* setsockoption */
	int	nbuf;
	ulong	buflen;
	int	sinkmode;	/* role */
	char	*whereTo;	/* hostname or IP address */
	ushort	port;		/* socket setting */
	int	udp;		/* transport mode : true udp, false tcp */
	ulong	bufalign;	/* hmm 1 */
	ulong	bufoffset;	/* hmm 2 */
	ulong	sockbufsize;	/* socket setting, maybe needs to be different type */
	char	fmt;		/* -f argument, whatever that is ??? */
	int	touchdata;
	int	maxtime;
	int	argc;		/* for now */
	char	**argv;		/* for now */
}	attcp_opt_t,	*attcp_opt_p;

typedef struct	attcp_conn {
	struct sockaddr_in
		sinHere, sinThere, frominet;
	struct hostent	*addr;
	char	*buf;	/* buffer for I/O read/write */
	int	sd;	/* socket descriptor */
	ulong	sockcalls;	/* track socket calls made - read or write */
	uint64_t	nbytes;		/* track connection bytes transferred */
}	attcp_conn_t,	*attcp_conn_p;

/* remove as global - better is as passed variable */
#ifdef GLOBALS
extern attcp_opt_p	a;
extern attcp_conn_p	c;
#endif

void err();
void mes();
int pattern();

void timer0();
void timer1();

void prep_timer();
double read_timer();
double elapsed_time();
int Nread(int sd, void*buf, unsigned count,
	attcp_conn_p     c,
	attcp_opt_p     a);
int Nwrite(int sd, void*buf, unsigned count,
	attcp_conn_p     c,
	attcp_opt_p     a);
void delay();
int mread(int sd, void*buf, unsigned count, attcp_conn_p     c);
char *outfmt(double b, char fmt);

typedef int boolean;
void attcp_rpt(boolean verbose, char fmt, uint64_t nbytes);

extern char Usage[];
boolean my_alarm;
