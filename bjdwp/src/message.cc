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

#include "debug.h"
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

	jdwplog("JDWMessage::RawSend: Socket ID " << m_socket
			<< "\nSent:\n" << send);
}


bool JDWMessage::RawReceive(Data &receive, int timeout)
{
	bool ret;

	ret = m_jdwp.Read(m_socket, receive, timeout);

	if (ret) 
		jdwplog("JDWMessage::RawReceive: Socket ID " << m_socket
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

