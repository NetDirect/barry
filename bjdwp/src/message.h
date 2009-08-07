#ifndef __BARRYJDWP_MESSAGE_H__
#define __BARRYJDWP_MESSAGE_H__


#include "jdwp.h"


namespace JDWP {

class JDWMessage {
protected:

private:
	int m_socket;

	JDWP::JDWP m_jdwp;

	void RawSend(Barry::Data &send, int timeout = -1);
	bool RawReceive(Barry::Data &receive, int timeout = -1);

public:
	JDWMessage(int socket);
	~JDWMessage();

	void Send(Barry::Data &send, int timeout = -1);	// send only
	void Send(Barry::Data &send, Barry::Data &receive, int timeout = -1); // send+recv
	bool Receive(Barry::Data &receive, int timeout = -1);
};

} // namespace JDWP

#endif

