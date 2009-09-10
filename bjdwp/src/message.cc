///
/// \file	message.cc
///		JDWP USB message implementation
///

/*
    Copyright (C) 2009, Nicolas VIVIEN

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <barry/barry.h>

#include "message.h"


using namespace Barry;


namespace JDWP {


JDWMessage::JDWMessage(int socket)
	: m_socket(socket)
{
}


JDWMessage::~JDWMessage()
{
}


void JDWMessage::RawSend(Data &send, int timeout)
{
	m_jdwp.Write(m_socket, send, timeout);

	barryverbose("JDWMessage::RawSend: Socket ID " << m_socket
			<< "\nSent:\n" << send);
}


bool JDWMessage::RawReceive(Data &receive, int timeout)
{
	bool ret;

	ret = m_jdwp.Read(m_socket, receive, timeout);

	if (ret) 
		barryverbose("JDWMessage::RawReceive: Socket ID " << m_socket
				<< "\nReceived:\n" << receive);

	return ret;
}


void JDWMessage::Send(Data &send, int timeout)
{
	RawSend(send, timeout);
}


void JDWMessage::Send(Data &send, Data &receive, int timeout)
{
	RawSend(send, timeout);
	RawReceive(receive, timeout);
}


bool JDWMessage::Receive(Data &receive, int timeout)
{
	return RawReceive(receive, timeout);
}

} // namespace JDWP

