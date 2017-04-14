/*
 *	A T T C P  -- C O N N E C T
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
	char *peername = NULL;
	unsigned long addr_tmp = 0L;

	struct sockaddr_in *sinHere  = &c->sinHere;
	struct sockaddr_in *sinThere = &c->sinThere;

	if(a_opts->x_flag || a_opts->c_flag)  { /* xmit */
		/* make connection to a host, either collect or as xmitter */
		if ((optind == a_opts->argc) && (a_opts->peername == NULL))
			usage();

		bzero((char *)sinThere, sizeof(*sinThere));
		bzero((char *)sinHere, sizeof(*sinThere));
		if (a_opts->peername == NULL)
			peername = a_opts->peername = a_opts->argv[optind];
		else
			peername = a_opts->peername;
		/* prepare msg for just in case ! */
		sprintf(message,"bad hostname:%s", peername);

		if (atoi(peername) > 0 )  {
			/* Numeric */
			sinThere->sin_family = addr_family;
			sinThere->sin_addr.s_addr = inet_addr(peername);
			if (sinThere->sin_addr.s_addr == INADDR_NONE)
				err(message);
		} else {
			struct hostent	*addr;
			if ((addr=gethostbyname(peername)) == NULL)
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

		if (getsockname(sd,  SA(&sock), &socklen) < 0) {
			err("getsockname");
		}
		if (a_opts->verbose > 3) {
			fprintf(stderr,"attcp: bind() on %s:%d\n", inet_ntoa(sock.sin_addr), ntohs(sock.sin_port));
		}
	}
	return(sd);
}

/*
 * set options on a socket
 */
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

/*
 * connect a socket
 */
attcp_connect( attcp_conn_p c, attcp_opt_p a_opts)
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
/*
 * need to reconsider when and how signals are caught
 *
		signal(SIGINT, sigintr);
		signal(SIGPIPE, sigpipe);
 */
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
			mes("accept");
		}
	}
}
