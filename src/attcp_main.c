/*
 *	A T T C P  -- M A I N
 *
 * Test TCP connection.
 * Make a connection on port PORT, PORTREAD, PORTSEND (32765, 32766, 32767)
 * and transfer fabricated buffers
 * Copyright 2013-2017: Michael Felt and AIXTOOLS.NET

 * $Date: 2017-04-12 20:08:59 +0000 (Wed, 12 Apr 2017) $
 * $Revision: 243 $
 * $Author: michael $
 * $Id: attcp_main.c 243 2017-04-12 20:08:59Z michael $
 */

#include <config.h>
#include <string.h>
#include "attcp.h"

/*
 * all routines will use the same option settings
 */
static attcp_opt_t      A;
static	attcp_opt_p      a_opts = &A;
static	boolean	x_flag;
static	boolean udp;

#ifdef XXXX
char stats[128];
unsigned long nbytes;           /* bytes on net */
unsigned long sockCalls;        /* # of socket() I/O calls */
double cput, realt;             /* user, real time (seconds) */
#endif

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
	syslog(LOG_INFO, "%s", s);
	fprintf(stdout,"mes(attcp%s: %s)\n", x_flag ? "-t":"-r", s);
	fflush(stdout);
}

/*
 * set global options - these will be inherited by threads
 */
attcp_opt_p attcp_getopt(int argc,char **argv)
{
	unsigned long addr_tmp;
	int opt;
	char m;
	attcp_opt_p	a_opts;
	/*
	 * first set the defaults
	 */
	A.buflen	= 8 * 1024;
	A.nbuf		= 20 * 1024;
	A.port		= PORT;
	A.udp		= 0;	/* TCP by default */
	A.threads	= 1;
	A.verbose	= 0;
	A.fmt		= 'M';
	/*	A.sinkmode	= TRUE; */
	/*
	 * will accept -h to set hostname to of peer to connect to
	 * also accept -c (with -r) to specify hostname
	 */
	A.peername	= NULL;

	A.maxtime	= 10;

	/*
	 * all other values default to zero or NULL
	 */

	if (argc < 2)
		usage();

	a_opts = &A;
	/*
	 * keep argc and argv for now, expect to delete
	 */
	a_opts->argc = argc;
	a_opts->argv = argv;

	while ((opt = getopt(argc, argv, ":dqruvxDTc:b:f:h:i:l:n:p:s:t:N:")) != -1) {
#ifdef DEBUG
		if (a_opts->verbose > 6) {
			fprintf(stderr, "opt:%c optarg:%s\n", opt, optarg);
			fprintf(stderr, "optind:%d argv[optind]:%s\n", optind, argv[optind]);
		}
#endif
		switch (opt) {

		case 'x':				/* transmit */
			a_opts->x_flag = 1;
			break;
		case 'r':				/* receive - must have "connect" */
			a_opts->x_flag = 0;
			break;
		case 'D':				/* SO_DEBUG */
			a_opts->options |= SO_DEBUG;
			break;
		case 'd':
#ifdef TCP_NODELAY
			a_opts->nodelay = 1;
#else
			a_opts->nodelay = 0;
			fprintf(stdout, "%s:%s\n", argv[0],
			    "-D option ignored: TCP_NODELAY socket option not supported");
#endif
			break;
		case 'n':				/* number of iops */
			a_opts->nbuf = atoi(optarg);
			m = tolower(optarg[strlen(optarg)-1]);
			if ((m == 'k') || (m == 'K'))
				a_opts->nbuf *= 1024;
			else if ((m == 'm') || (m == 'M'))
				a_opts->nbuf *= 1024 * 1024;
			break;
		case 'l':
			fprintf(stdout, "%s:%s\n", argv[0],
			    "-l option degraded: use -b instead");
		case 'b':
			a_opts->buflen = atoi(optarg);

			m = tolower(optarg[strlen(optarg)-1]);
			if ((m == 'k') || (m == 'K'))
				a_opts->nbuf *= 1024;
			else if ((m == 'm') || (m == 'M'))
				a_opts->nbuf *= 1024 * 1024;
			break;
		case 't':				/* Threads count */
			a_opts->threads = atoi(optarg);
			break;
		case 'p':				/* peer port */
			a_opts->port = atoi(optarg);
			break;
		case 'u':				/* UDP rather that TCP */
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
			if ((m == 'k') || (m == 'K'))
				a_opts->nbuf *= 1024;
			else if ((m == 'm') || (m == 'M'))
				a_opts->nbuf *= 1024 * 1024;
#else
			fprintf(stdout, 
			    "ttcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
#endif
			break;
		case 'f':				/* set format for stdout, no-verbose */
			a_opts->fmt = *optarg;
			break;
		case 'T':				/* Touch data - i.e., do some work with data before/after i/o */
			a_opts->touchdata = 1;
			break;
		case 'c':				/* connect flag (receive mode! */
			a_opts->c_flag = 1;
		case 'h':				/* set peer hostname or IP address */
			a_opts->peername = optarg;
			break;
		case 'q':				/* toggle quiet mode */
			a_opts->q_flag = !a_opts->q_flag;
			break;
		case 'v':				/* add in verbosity */
			a_opts->verbose++;
			break;
		case 'i':				/* interval/time to send/receive iops */
			a_opts->maxtime = atoi(optarg);
			if (a_opts->maxtime < 0)
				err("interval must be positive");
			break;
		case 'N':				/* max number of bytes to send/receive per thread */
			a_opts->maxbytes = atoi(optarg);
			if (a_opts->maxbytes < 0)
				err("maxbytes must be positive");
			break;
		default:
			usage();
		}
	}
	return(a_opts);
}

main(int argc, char **argv)
{
	/*
	 * static for now, need to come from malloc() in the future
	 */
	char	name[256];

	attcp_opt_p      ax;
	attcp_conn_p     c;
	int		sd; /* socket descriptor */

	/*
	 * get the global variables
	 */
	ax = a_opts = attcp_getopt(argc,argv);

	x_flag = ax->x_flag;	/* for message routines to set -t or -r */
	udp = (ax->udp != 0);	/* for various routines, mainly read/write */

	attcp_log_init(argv[0]);

	/*
	if (!attcp_pthread_init(ax->threads) {
		fprintf(stderr, "thread data init failed\n");
		exit(-1);
	}
	if (!attcp_pthread_socket(ax->threads, a_opts) {
		fprintf(stderr, "thread socket creation failed\n");
		exit(-1);
	}
	if (!attcp_pthread_start(ax->threads) {
		fprintf(stderr, "pthread creation failed\n");
		exit(-1);
	}
	 * atm - mono-threaded - static Connection structure
	 */
	/*
	 * set socket options in _socket()
	 */
	sd = attcp_socket(c, a_opts);
	if (c->sd != sd) {
		fprintf(stderr, "socket creation failed\n");
		exit(-1);
	}
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_socket()\n");

	attcp_log_opts(a_opts);
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_log_opts()\n");

	attcp_setoption( c, a_opts);
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_setoption()\n");

	fflush(stdout);
	fflush(stderr);

	attcp_connect( c, a_opts);
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_connect()\n");

	attcp_log_name(c->sd, name, ax->verbose);
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_log_name()\n");

	attcp_xfer( c, a_opts);
	if (ax->verbose > 5)
		fprintf(stderr,"finished attcp_xfer()\n");

	close (c->sd);
	if (ax->verbose > 5)
		fprintf(stderr,"finished close() of socket\n");

	sockCalls += c->sockcalls;

	attcp_rpt(ax->verbose,ax->fmt,c->nbytes);
	exit(0);
}
