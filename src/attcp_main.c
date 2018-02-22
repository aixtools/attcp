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
static	boolean	x_flag;

void
sigalarm( int x)
{
	mes("alarm");
}
void
sigintr( int x)
{
	/*
	attcp_report();
	attcp_rpt(TRUE,fmt,nbytes);
	*/
	mes("SIGINT received");
	attcp_thread_stop(A.threads);
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
	fprintf(stderr,"err:attcp%s: ", x_flag?"-xmit":"-rcvr");
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
	fprintf(stdout,"mes(attcp%s: %s)\n", x_flag ? "-xmit":"-rcvr", s);
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
	attcp_opt_p	a_opts  = &A;
	attcp_set_p	cs = &A.settings;

	if (argc < 2) {
		mes("< 2 args");
		usage();
	}

	/*
	 * first set the defaults
	 */
	A.threads	= 1;
	A.peername	= NULL;
	A.port		= PORT;
	A.fmt		= 'M';

	cs->c_flag	= 0;
	cs->x_flag	= 0;
	cs->q_flag	= 0;
	cs->verbosity	= 0;
	cs->transport	= A_TCP;	/* TCP by default */
	cs->io_count	= 20 * 1024;
	cs->maxtime	= 10;
	cs->io_size	= (uint64_t) (8 * 1024);
	cs->maxbytes	= cs->io_size * 1024 * 1024;

	/*
	 * all other values default to zero or NULL
	 */
	cs->sockbufsize = 0;
	cs->so_options	= 0;
	cs->no_delay	= 0;
	cs->touchdata	= 0;
	cs->B_flag	= 0;
	cs->role	= 0;

	while ((opt = getopt(argc, argv, ":dqruvxDTc:b:f:h:i:l:n:p:s:t:N:")) != -1) {
#ifdef DEBUG
		if (a_opts->verbosity > 6) {
			fprintf(stderr, "opt:%c optarg:%s\n", opt, optarg);
			fprintf(stderr, "optind:%d argv[optind]:%s\n", optind, argv[optind]);
		}
#endif
		switch (opt) {
		case 'x':				/* transmit */
			cs->x_flag = 1;
			break;
		case 'r':				/* receive - must have "connect" */
			cs->x_flag = 0;
			break;
		case 'D':				/* SO_DEBUG */
			cs->so_options |= SO_DEBUG;
			break;
		case 'd':
#ifdef TCP_NODELAY
			cs->no_delay = 1;
#else
			cs->nodelay = 0;
			fprintf(stdout, "%s:%s\n", argv[0],
			    "-D option ignored: TCP_NODELAY socket option not supported");
#endif
			break;
		case 'n':				/* number of iops */
			cs->io_count = atoi(optarg);
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				cs->io_count *= 1024;
			else if (m == 'm')
				cs->io_count *= 1024 * 1024;
			break;
		case 'l':
			fprintf(stdout, "%s:%s\n", argv[0],
			    "-l option degraded: use -b instead");
		case 'b':
			cs->io_size = atoi(optarg);

			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				cs->io_size *= 1024;
			else if (m == 'm')
				cs->io_size *= 1024 * 1024;
			break;
		case 't':				/* Threads count */
			a_opts->threads = atoi(optarg);
			break;
		case 'p':				/* peer port */
			a_opts->port = atoi(optarg);
			break;
		case 'u':				/* UDP rather that TCP */
			cs->transport = A_UDP;
			break;
		case 's':
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
			cs->sockbufsize = atoi(optarg);
			if (cs->sockbufsize <= 0) {
				char msg[256];
				sprintf(msg,"-s %s: bad socketsize: cs->socketbufsize == %d",optarg,cs->sockbufsize);
				err(msg);
			}
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				cs->sockbufsize *= 1024;
			else if (m == 'm')
				cs->sockbufsize *= 1024 * 1024;
#else
			fprintf(stdout, 
			    "ttcp: -b option ignored: SO_SNDBUF/SO_RCVBUF socket options not supported\n");
#endif
			break;
		case 'f':				/* set format for stdout, no-verbose */
			a_opts->fmt = *optarg;
			break;
		case 'T':				/* Touch data - i.e., do some work with data before/after i/o */
			cs->touchdata = 1;
			break;
		case 'c':				/* connect flag (receive mode! */
			cs->c_flag = 1;
		case 'h':				/* set peer hostname or IP address */
			a_opts->peername = optarg;
#ifdef VERBOSE
			fprintf(stderr,"A.peername = %s %lp\n", A.peername, a_opts->peername);
#endif
			break;
		case 'q':				/* toggle quiet mode */
			cs->q_flag = !cs->q_flag;
			break;
		case 'v':				/* add in verbosity */
			cs->verbosity++;
			break;
		case 'i':				/* interval/time to send/receive iops */
			cs->maxtime = atoi(optarg);
			if (cs->maxtime < 0)
				err("interval must be positive");
			break;
		case 'N':				/* max number of bytes to send/receive per thread */
			cs->maxbytes = atoi(optarg);
			if (cs->maxbytes < 0)
				err("maxbytes must be positive");
			m = tolower(optarg[strlen(optarg)-1]);
			if (m == 'k')
				cs->maxbytes *= 1024;
			else if (m == 'm')
				cs->maxbytes *= 1024 * 1024;
			else if (m == 'g')
				cs->maxbytes *= 1024 * 1024 * 1024;
			break;
		default:
			fprintf(stderr,"%c is not seen as an option\n", opt);
			usage();
		}
	}
	return(a_opts);
}

void
main(int argc, char **argv)
{
	/*
	 * static for now, need to come from malloc() in the future
	 */
	char		buffer[256];
	pthread_t	last_tid;

	attcp_opt_p      ax;
	attcp_conn_p     c;
	uint64_t	nbytes;

	/*
	 * get the global variables
	 */
	ax = attcp_getopt(argc,argv);
	attcp_log_init(argv[0]);
	attcp_log_opts(ax);
	if (ax->settings.verbosity > 5)
		mes("finished attcp_log_opts()");

	x_flag = ax->settings.x_flag;	/* for message routines to set -t or -r */
#ifdef MONOTHREAD
	udp = (ax->udp != 0);	/* for various routines, mainly read/write */
#endif

	if (!attcp_pthread_init(ax->threads)) {
		err("thread data init failed");
	}
/*
	if (!attcp_pthread_socket(ax)) {
		err("pthread socket creation failed");
	}
*/
	attcp_pthread_socket(ax);
/*
 * need to set signals to ignore during thread creation
 * only the main thread is to react - but docs contradict how threads
 * respond to changing signal vectors - hmm.
 */
	signal(SIGINT, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	last_tid = attcp_pthread_start(ax->threads);
	if (last_tid == (pthread_t) NULL) {
		err("pthread start failed");
	}
/*
 * may need to reconsider when and how signals are caught
 */
	signal(SIGINT, sigintr);
	signal(SIGALRM, sigalarm);

	pthread_join(last_tid, NULL);

	nbytes = attcp_thread_done(ax->threads);

	attcp_rpt(ax->settings.verbosity,ax->fmt,nbytes);

	exit(0);
}
