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
	struct sockaddr_in *sinPeer = &c->sinPeer;

	bzero((char *)sinHere, sizeof(*sinHere));

	if ((sd = socket(addr_family, a_opts->udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
		err("socket");
	if(a_opts->x_flag | a_opts->c_flag)  { /* xmit or call */
		sinHere->sin_port = 0;		/* free choice */
	} else { /* rcvr */
		sinHere->sin_port =  htons(a_opts->port);
	}
	if (bind(sd, SA(sinHere), sizeof(*sinHere)) < 0)
		err("bind");
	c->sd = sd;
	socket_cnt++;

	/*
	 * setup sinPeer is xmit or call
	 */
	if(a_opts->x_flag | a_opts->c_flag)  { /* xmit or call */
		if (a_opts->peername == NULL)
			usage();

		peername = a_opts->peername;
		bzero((char *)sinPeer, sizeof(*sinPeer));

		/* prepare msg for just in case ! */
		sprintf(message,"bad hostname:%s", peername);

		if (atoi(peername) > 0 )  {	/* Numeric IP Addr */
			sinPeer->sin_family = addr_family;
			sinPeer->sin_addr.s_addr = inet_addr(peername);
			if (sinPeer->sin_addr.s_addr == INADDR_NONE)
				err(message);
		} else {			/* textual name */
			struct hostent	*addr;
			if ((addr=gethostbyname(peername)) == NULL)
				err(message);
			sinPeer->sin_family = addr->h_addrtype;
			bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
			sinPeer->sin_addr.s_addr = addr_tmp;
		}
		sinPeer->sin_port = htons(a_opts->port);
	}
	/*
	 * if not xmit or call, sinPeer is filled when we accept()
	 */
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
		if (a_opts->verbose > 4)	{
			fprintf(stderr, "\nsockbufsize: %0x", a_opts->sockbufsize);
			fprintf(stderr, "\nsockbufsize: x_flag:%d\n", a_opts->x_flag);
		}
		if (a_opts->x_flag) {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_SNDBUF,
			    &a_opts->sockbufsize, sizeof(a_opts->sockbufsize)) < 0)
				err("setsockopt: sndbuf");
			if (a_opts->verbose > 4)
				mes("sndbuf");
		} else {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_RCVBUF,
			    &a_opts->sockbufsize, sizeof(a_opts->sockbufsize)) < 0)
				err("setsockopt: rcvbuf");
			if (a_opts->verbose > 4)
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
void
attcp_connect( attcp_conn_p c, attcp_opt_p a_opts)
{
	register char *buf;
	int	sd = c->sd;
	register int nbuf = a_opts->nbuf;
	register int buflen = a_opts->buflen;
	struct	sockaddr_in	*sinPeer = &c->sinPeer;

	if (a_opts->udp && a_opts->buflen < 5) {
		a_opts->buflen = 5;		/* send more than the sentinel size */
	}

	if ( (buf = (char *)malloc(a_opts->buflen)) == (char *)NULL)
		err("malloc");
	c->buf = buf;

	if (a_opts->udp)
		return;
	/*
	 * TCP - either connect with peer
	 * or listen() - waiting for a peer to request a connection
	 * - connect() is what is expected - with the inetd.conf waiting
	 */
	if (a_opts->x_flag || a_opts->c_flag) {		/* connect with peer */
		/* We are the client if transmitting */
		if(connect(sd, SA(sinPeer), sizeof(*sinPeer) ) < 0)
			err("connect");
		/* if (!a_opts->q_flag) mes("connect"); */
	} else {					/* listen/wait for connection */
		struct sockaddr_in	*peer;
		socklen_t	peerlen;
		peer = &c->sinPeer;

		/*
		 * otherwise, we are the server and 
		 * should listen for the connections
		 */
		if (!a_opts->q_flag)
			mes("listen");
		listen(sd,0);   /* allow a queue of 0 */
		/* options = 0 || SO_DEBUG */
		if(a_opts->options)  {
			int one = 1;
			if( setsockopt(sd, SOL_SOCKET, a_opts->options,
					&one, sizeof(one)) < 0)
				err("setsockopt");
		}
		peerlen = sizeof(*peer);
		if((sd=accept(sd, SA(peer), &peerlen) ) < 0)
			err("accept");
		if (!a_opts->q_flag)
			mes("accept");
	}
}
