///
/// \file	j_message.cc
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

#include "j_message.h"
#include "debug.h"
#include "data.h"


namespace Barry { namespace JDWP {


JDWMessage::JDWMessage(int socket)
	: m_socket(socket)
{
}


JDWMessage::~JDWMessage()
{
}


void JDWMessage::RawSend(Data &send, int timeout)
{
	bool ret = m_jdwp.Write(m_socket, send, timeout);

	if (ret)
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

}} // namespace Barry::JDWP

