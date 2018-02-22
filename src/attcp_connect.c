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
void
attcp_socket(attcp_conn_p c, attcp_opt_p a)
{
	attcp_set_p	opts = &c->settings;
	static 	int	socket_cnt = 0;
	char message[128];
	int addr_family = AF_INET;
	int sd = -1;
	char *peername = NULL;
	unsigned long addr_tmp = 0L;

	struct sockaddr_in *sinHere  = &c->sinHere;
	struct sockaddr_in *sinPeer = &c->sinPeer;

	bzero((char *)sinHere, sizeof(*sinHere));

	if ((sd = socket(addr_family, opts->transport, 0)) < 0)
		err("socket");
	if(opts->x_flag | opts->c_flag)  { /* xmit or call */
		sinHere->sin_port = 0;		/* free choice */
	} else { /* rcvr */
		sinHere->sin_port =  htons(a->port);
	}
	if (bind(sd, SA(sinHere), sizeof(*sinHere)) < 0)
		err("bind");
	c->sd = sd;
	socket_cnt++;

	/*
	 * setup sinPeer is xmit or call
	 */
	if(opts->x_flag | opts->c_flag)  { /* xmit or call */
		if (a->peername == NULL) {
			mes("peername is NULL");
			usage();
		}

		peername = a->peername;
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
		sinPeer->sin_port = htons(a->port);
	}
}

/*
 * set options on a socket
 */
void
attcp_setoption( attcp_conn_p c)
{
	int sd = c->sd;
	attcp_set_p	opts = &c->settings;
	int one = 1;
#if defined(SO_SNDBUF) || defined(SO_RCVBUF)
	if (opts->sockbufsize) {
		if (opts->verbosity > 4)	{
			fprintf(stderr, "\nsockbufsize: %0x", opts->sockbufsize);
			fprintf(stderr, "\nsockbufsize: x_flag:%d\n", opts->x_flag);
		}
		if (opts->x_flag) {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_SNDBUF,
			    &opts->sockbufsize, sizeof(opts->sockbufsize)) < 0)
				err("setsockopt: sndbuf");
			if (opts->verbosity > 4)
				mes("sndbuf");
		} else {
			if (setsockopt(sd,
			    SOL_SOCKET, SO_RCVBUF,
			    &opts->sockbufsize, sizeof(opts->sockbufsize)) < 0)
				err("setsockopt: rcvbuf");
			if (opts->verbosity > 4)
				mes("rcvbuf");
		}
	}
#endif
	if (opts->so_options)  {
		if( setsockopt(sd,
		    SOL_SOCKET, opts->so_options,
		    &one, sizeof(one)) < 0)
			err("setsockopt");
	}
#ifdef TCP_NODELAY
	if (opts->no_delay) {
		struct protoent *p;
		p = getprotobyname("tcp");
		if( p && setsockopt(sd, p->p_proto, TCP_NODELAY, 
		    &one, sizeof(one)) < 0)
			err("setsockopt: no_delay");
		mes("no_delay");
	}
#endif
}

/*
 * connect a socket
 */
void
attcp_connect( attcp_conn_p c)
{
	int	sd = c->sd;
	attcp_set_p	opts = &c->settings;
	struct sockaddr_in	*peer;
	socklen_t	peerlen = sizeof(*peer);
	peer = &c->sinPeer;

	if (opts->transport == SOCK_DGRAM) {
		/* must send more than the sentinel size */
		if (opts->io_size < 5)
			opts->io_size = 5;
		/* connectionless, done for now */
		return;
	}

	/*
	 * connect with specified peer endpoint - should be TCP
	 * or listen() - waiting for a peer to request a connection
	 * - connect() is what is expected - with the inetd.conf waiting
	 */
	if (opts->x_flag || opts->c_flag) {		/* connect with peer */
		if(connect(sd, SA(peer), peerlen) < 0)
			err("connect");
	} else {	/* listen/wait for connection */

		struct sockaddr_in	peer;
		/*
		 * otherwise, we are the server and 
		 * should listen for the connections
		 */
		if (!opts->q_flag)
			mes("listen");
		listen(sd,0);   /* allow a queue of 0 */
		if (!opts->q_flag)
			mes("heard");
		if((sd=accept(sd, SA(&peer), &peerlen) ) < 0)
			err("accept failed");
		if (!opts->q_flag)
			mes("accept OK");
		if(opts->so_options)  { /* set options on new socket */
			int one = 1;
			if( setsockopt(sd, SOL_SOCKET, opts->so_options,
			    &one, sizeof(one)) < 0)
				err("setsockopt after accept()");
		}
		c->sd = sd;
		/*
		 * could actually verify the connections
		 * but this will work with 'attcp' setup as a daemon
		 */
		c->sinPeer = peer;
	}
}
