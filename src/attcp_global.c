/*
 *	A T T C P  -- M A I N
 *
 * Test TCP connection.  Makes a connection on port 55000,55001,55002
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

struct sockaddr_in sinme;
struct sockaddr_in sinhim;
struct sockaddr_in frominet;

int domain;
socklen_t fromlen;
int sd;				/* sd of network socket */

int buflen = 8 * 1024;		/* length of buffer */
char *buf;			/* ptr to dynamic buffer */
int nbuf = 2 * 1024;		/* number of buffers to send in sinkmode */

int bufoffset = 0;		/* align buffer to this */
int bufalign = 16*1024;		/* modulo this */

int udp = 0;			/* 0 = tcp, !0 = udp */
int options = 0;		/* socket options */
int one = 1;                    /* for 4.3 BSD style setsockopt() */
unsigned short port = 55000;		/* TCP port number */
char *host;			/* ptr to name of host */
int trans;			/* 0=receive, !0=transmit mode */
int sinkmode = 0;		/* 0=normal I/O, !0=sink/source mode */
int verbose = 0;		/* 0=print basic info, 1=print cpu rate, proc
				 * resource usage. */
int nodelay = 0;		/* set TCP_NODELAY socket option */
int b_flag = 0;			/* use mread() */
int sockbufsize = 0;		/* socket buffer size to use */
char fmt = 'K';			/* output format: k = kilobits, K = kilobytes,
				 *  m = megabits, M = megabytes, 
				 *  g = gigabits, G = gigabytes */
int touchdata = 0;		/* access data after reading */
int c_flag = 0;		/* collect call mode */
int q_flag = 0;		/* keep quiet */
int interval = 0;		/* time interval to test over */

struct hostent *addr;
extern int errno;
extern int optind;
extern char *optarg;

char Usage[] = "\
Usage: attcp -x [-options] [-h] host ## transmit (xmit)\n\
       attcp -r [-options] [-c host] ## read from network\n\
Common options:\n\
        -x      source a pattern to network\n\
        -r      read as datasink (discard) all data from network\n\
        -c host \"collect call\": initiate connection with host then receive data\n\
        -h host set hostname to connect with\n\
        -l ##   length of bufs read from or written to network (default 8192)\n\
        -n ##   number of source bufs written to network (default 20480, or 20k) \n\
        -i ##   \"interval\": number of seconds to run the test: rather than # of buffers (see -n)\n\
                using defaults only - 20k * 8k (or 160M) bytes is transferred per thread\n\
        -t ##   number of threads (default is 1)\n\
        -p ##   port number to send to or listen at (default 49149)\n\
        -u      use UDP instead of TCP\n\
        -d      don't buffer TCP writes (sets TCP_NODELAY socket option)\n\
        -D      set SO_DEBUG socket option\n\
        -s ##   set socket buffer size (if supported)\n\
        -f X    format for rate: b,B = {bit,byte}; k,K = kilo{bit,byte};\n\
                m,M = mega; g,G = giga; r,R = raw {bit,byte}\n\
        -q      \"quiet\": only print the measured throughput, emit no other chatty output.\n\
        -v[vvv] verbose: print more statistics\n\
Options specific to -r:\n\
        -B      process full blocks as specified by -l (concate reads until \"blocksize\" is reached\n\
        -T      \"touch\": access each byte as it's read\n\
";


char stats[128];
unsigned long nbytes;		/* bytes on net */
unsigned long sockCalls;	/* # of socket() I/O calls */
double cput, realt;		/* user, real time (seconds) */
