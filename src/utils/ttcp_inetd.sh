#!/usr/bin/sh
# install services into /etc/services and /etc/inetd.conf
# Copyright Michael Felt and AIXTOOLS, 2013-2017

# $Date: 2016-01-17 11:44:03 +0000 (Sun, 17 Jan 2016) $
# $Revision: 178 $
# $Author: michael $
# $Id: ttcp_inetd.ksh 178 2016-01-17 11:44:03Z michael $

grep 32765 /etc/services
if [[ $? -ne 0 ]]
then
	echo "ttcpr     32765/tcp # ttcp_reader or sink" >>/etc/services
	echo "ttcpr     32765/udp # ttcp_reader or sink" >>/etc/services
	echo "ttcpr     stream  tcp    nowait  nobody    /opt/bin/ttcpr     ttcpr" >>/etc/inetd.conf
fi

grep 32766 /etc/services
if [[ $? -ne 0 ]]
then
	echo "ttcps     32766/tcp # ttcp_sender faucet" >>/etc/services
	echo "ttcps     32766/udp # ttcp_sender faucet" >>/etc/services
	echo "ttcps     stream  tcp    nowait  nobody    /opt/bin/ttcps     ttcps" >>/etc/inetd.conf
fi

grep 32767 /etc/services
if [[ $? -ne 0 ]]
then
	echo "ttcp     32767/tcp # ttcp port holder" >>/etc/services
	echo "ttcp     32767/udp # ttcp port holder" >>/etc/services
fi

refresh -s inetd
