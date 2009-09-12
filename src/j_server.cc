///
/// \file	j_server.cc
///		Server protocol implementation
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

#include "j_server.h"
#include "protocol.h"
#include "data.h"
#include "endian.h"
#include "debug.h"
#include "j_message.h"
#include "protostructs.h"
#include "record-internal.h"
#include "error.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include <sstream>
#include <algorithm>

using namespace std;

namespace Barry { namespace JDWP {

static void * acceptThread(void *data);


JDWServer::JDWServer(Barry::Mode::JVMDebug &device,
			const char *address, int port)
	: jvmdebug(&device)
	, acceptfd(-1)
	, sockfd(-1)
	, address(address)
	, port(port)
	, loop(false)
	, targetrunning(false)
	, printConsoleMessage(0)
{
	SearchDebugFile(debugFileList);
}


JDWServer::~JDWServer() 
{
	Stop();
}


void JDWServer::SetPasswordDevice(string password)
{
	this->password = password;
}


void JDWServer::SetConsoleCallback(ConsoleCallbackType callback)
{
	printConsoleMessage = callback;
}

static const char* h_strerror(int code)
{
	// Codes and messages taken from the Linux gethostbyname(3) manpage
	switch( code )
	{
	case HOST_NOT_FOUND:
		return "HOST_NOT_FOUND: The specified host is unknown";

	case NO_ADDRESS:
		return "NO_ADDRESS: The requested name is valid but does not have an IP address";

	case NO_RECOVERY:
		return "NO_RECOVERY: A non-recoverable name server error occurred";

	case TRY_AGAIN:
		return "TRY_AGAIN: A temporary error occurred on an authoritative name server. Try again later.";

	default:
		return "Unknown network error code";
	}
}

bool JDWServer::Start() 
{
	int rc;

	struct hostent *hp;
	struct sockaddr_in sad;


	memset((char *) &sad, '\0', sizeof(struct sockaddr_in));

	if (!address.size())
		sad.sin_addr.s_addr = INADDR_ANY;
	else {
		sad.sin_addr.s_addr = inet_addr(address.c_str());

		if (sad.sin_addr.s_addr == INADDR_NONE) {
			hp = gethostbyname(address.c_str());

			if (hp == NULL) {
				std::ostringstream oss;
				oss << "JDWServer::Start: " << h_errno << h_strerror(h_errno);
				throw Barry::Error(oss.str());
			}

			memcpy((char*) &sad.sin_addr, (char*) hp->h_addr, (size_t) hp->h_length);
		}
	}

	sad.sin_family = AF_INET;
	sad.sin_port = htons((short) (port & 0xFFFF));

	// Open socket
	sockfd = socket(sad.sin_family, SOCK_STREAM, 0);

	if (sockfd < 0) {
		throw Barry::ErrnoError("JDWServer::Start: Cannot open socket.", errno);
	}

	// Bind
	rc = bind(sockfd, (struct sockaddr *) &sad, sizeof(sad));

	if (rc < 0) {
		int code = errno;

		close(sockfd);
		sockfd = -1;

		throw Barry::ErrnoError("JDWServer::Start: Cannot bind socket", code);
	}

	// Listen
	if (listen(sockfd, SOMAXCONN) < 0) {
		int code = errno;

		close(sockfd);
		sockfd = -1;

		throw Barry::ErrnoError("JDWServer::Start: Cannot listen on socket", code);
	}

	handler.reset(new Thread(sockfd, acceptThread, (void*) this));

	return true;
}


static void * acceptThread(void *data)
{
	JDWServer *s;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	s = (JDWServer *) data;

	while (1) {
		if( s->AcceptConnection() &&
			s->AttachToDevice() &&
			s->InitVisibleClassList() &&
			s->Hello() )
		{
			s->Run();
			s->DetachFromDevice();
		}
	}

	return NULL;
}


// Returns true if a new connection was accepted and established
bool JDWServer::AcceptConnection()
{
	struct sockaddr_in addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t addrlen = sizeof(addr);

	acceptfd = accept(sockfd, sa, &addrlen);
	if( acceptfd < 0 )
		return false;

	fcntl(acceptfd, F_SETFL, O_NONBLOCK);
	return true;
}


bool JDWServer::AttachToDevice()
{
	targetrunning = false;

	jvmdebug->Open(password.c_str());
	jvmdebug->Attach();

	jvmdebug->Unknown01();
	jvmdebug->Unknown02();
	jvmdebug->Unknown03();
	jvmdebug->Unknown04();
	jvmdebug->Unknown05();

	jvmdebug->GetModulesList(modulesList);
	dout(modulesList);

	// Check debug info for each modules
	JVMModulesList::const_iterator b = modulesList.begin();
	for ( ; b != modulesList.end(); b++) {
		JDG::CodInfo codInfo;

		const JVMModulesEntry &entry = *b;

		bool ret = LoadDebugInfo(debugFileList, entry.UniqueID, entry.Name, codInfo);

		if (ret == true) {
			appList[entry.UniqueID].Load(codInfo);
		}
		else {
			dout("No debug information found for '" << entry.Name);
			dout("' (" << hex << setfill('0') << setw(8) << entry.UniqueID << ")." << endl)
		}
	}

	return true;
}


void JDWServer::DetachFromDevice()
{
	jvmdebug->Detach();
	jvmdebug->Close();
}


#define JDWP_HELLO_STRING		"JDWP-Handshake"



bool JDWServer::Hello()
{
	bool ret;

	Barry::Data response;
	
	const size_t len = strlen(JDWP_HELLO_STRING);

	JDWMessage msg(acceptfd);

	do {
		ret = msg.Receive(response);
	}
	while (!ret);

	size_t size = response.GetSize();
	char *str = (char *) response.GetBuffer();

	if (size != len)
		return false;
	
	if (!strncmp(str, JDWP_HELLO_STRING, len)) {
		Data command(JDWP_HELLO_STRING, len);

		msg.Send(command);

		return true;
	}

	return false;
}


void JDWServer::Run()
{
	string str;
	JDWMessage msg(acceptfd);

	Barry::Data command;

	MAKE_JDWPPACKET(rpack, command);

	loop = true;

	while (loop) {
		if (targetrunning) {
			// Read JDWP message from device
			int value = jvmdebug->GetConsoleMessage(str);

			if (value < 0) {
				bool ret;
				int status;

				ret = jvmdebug->GetStatus(status);

				while (!ret) {
					// Read JDB message from host
					msg.Receive(command);
			
					if (command.GetSize() > 0) {
						// Convert to packet
						rpack = (const Barry::Protocol::JDWP::Packet *) command.GetData();

						if (command.GetSize() != be_btohl(rpack->length)) {
							dout("Packet size error !!!" << endl);

							// TODO : add throw exception

							continue;
						}

						CommandsetProcess(command);
	
						break;
					}
					else
						ret = jvmdebug->WaitStatus(status);
				}
			}
			else {
				if (printConsoleMessage != NULL)
					printConsoleMessage(str);
			}
		}
		else {
			// Read JDB message from host
			msg.Receive(command);

			if (command.GetSize() > 0) {
				// Convert to packet
				rpack = (const Barry::Protocol::JDWP::Packet *) command.GetData();

				if (command.GetSize() != be_btohl(rpack->length)) {
					dout("Packet size error !!!" << endl);

					// TODO : add throw exception

					continue;
				}

				CommandsetProcess(command);
			}

			usleep(50);
		}
	}
}


bool JDWServer::Stop()
{
	if( handler.get() ) {
		handler->Dispose();
		handler.reset();
	}

	if( sockfd >= 0 ) {
		close(sockfd);
		sockfd = -1;
	}

	if( acceptfd >= 0 ) {
		close(acceptfd);
		acceptfd = -1;
	}

	return true;
}


bool JDWServer::InitVisibleClassList()
{
	int index;

	// Skip the cell '0'
	// it's very ugly, but I want use an index started at '1' inside of '0'
	// JDB works from '1' :(
	JDG::ClassEntry e;
	visibleClassList.push_back(e);

	// Count and index the class (start to '1')
	index = 1;
	JDWAppList::iterator it;

	for (it = appList.begin(); it != appList.end(); it++) {
		JDWAppInfo &appInfo = it->second;
		JDG::ClassList &list = appInfo.classList;
	
		JDG::ClassList::iterator b;

		for (b = list.begin(); b != list.end(); b++) {
			// FIXME
			// I don't from class field, we have to filter the class view by JDB
//			if ((b->type != 0x824) && (b->type != 0x64)) {
			if (b->id == 0xffffffff) {
				b->index = -1;

				continue;
			}

			b->index = index;

			visibleClassList.push_back(*b);

			index++;
		}
	}

	visibleClassList.CreateDefaultEntries();

	return true;
}


void JDWServer::CommandsetProcess(Data &cmd)
{
	MAKE_JDWPPACKET(rpack, cmd);

	switch (rpack->u.command.commandset) {
		case JDWP_CMDSET_VIRTUALMACHINE:
			CommandsetVirtualMachineProcess(cmd);
			break;

		case JDWP_CMDSET_REFERECENTYPE:
			break;

		case JDWP_CMDSET_CLASSTYPE:
			break;

		case JDWP_CMDSET_ARRAYTYPE:
			break;

		case JDWP_CMDSET_INTERFACETYPE:
			break;

		case JDWP_CMDSET_METHOD:
			break;

		case JDWP_CMDSET_FIELD:
			break;

		case JDWP_CMDSET_OBJECTREFERENCE:
			break;

		case JDWP_CMDSET_STRINGREFERENCE:
			break;

		case JDWP_CMDSET_THREADREFERENCE:
			break;

		case JDWP_CMDSET_THREADGROUPREFERENCE:
			break;

		case JDWP_CMDSET_ARRAYREFERENCE:
			break;

		case JDWP_CMDSET_CLASSLOADERREFERENCE:
			break;

		case JDWP_CMDSET_EVENTREQUEST:
			CommandsetEventRequestProcess(cmd);
			break;

		case JDWP_CMDSET_STACKFRAME:
			break;

		case JDWP_CMDSET_CLASSOBJECTREFERENCE:
			break;

		case JDWP_CMDSET_EVENT:
			break;

		default:
			// TODO : add exception (or alert)
			dout("Commandset unknown !!!" << endl);
	}
}


void JDWServer::CommandsetVirtualMachineProcess(Data &cmd)
{
	MAKE_JDWPPACKET(rpack, cmd);

	switch (rpack->u.command.command) {
		case JDWP_CMD_VERSION:
			CommandVersion(cmd);
			break;

		case JDWP_CMD_ALLCLASSES:
			CommandAllClasses(cmd);
			break;

		case JDWP_CMD_ALLTHREADS:
			CommandAllThreads(cmd);
			break;

		case JDWP_CMD_DISPOSE:
			loop = false;
			targetrunning = false;
			close(acceptfd);
			acceptfd = -1;
			break;

		case JDWP_CMD_IDSIZES:
			CommandIdSizes(cmd);
			break;

		case JDWP_CMD_SUSPEND:
			CommandSuspend(cmd);
			targetrunning = false;
			break;

		case JDWP_CMD_RESUME:
			CommandResume(cmd);
			targetrunning = true;
			break;

		case JDWP_CMD_CLASSPATHS:
			CommandClassPaths(cmd);
			break;
	}
}


void JDWServer::CommandsetEventRequestProcess(Data &cmd)
{
	MAKE_JDWPPACKET(rpack, cmd);

	switch (rpack->u.command.command) {
		case JDWP_CMD_SET:
			CommandSet(cmd);
			break;
	}
}


void JDWServer::CommandVersion(Data &cmd)
{
	JDWMessage msg(acceptfd);

	// Build packet data
	Data response;

	size_t offset = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	AddJDWString(response, offset, string("RIM JVM"));
	AddJDWInt(response, offset, be_htobl(1));
	AddJDWInt(response, offset, be_htobl(4));
	AddJDWString(response, offset, string("1.4"));
	AddJDWString(response, offset, string("RIM JVM"));

	response.ReleaseBuffer(offset);


	size_t total_size = response.GetSize();

	// Fill in the header values
	MAKE_JDWPPACKETPTR_BUF(cpack, response.GetBuffer(total_size));
	Barry::Protocol::JDWP::Packet &packet = *cpack;


	MAKE_JDWPPACKET(rpack, cmd);

	packet.length = be_htobl(total_size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	response.ReleaseBuffer(total_size);
	msg.Send(response);
}


void JDWServer::CommandAllClasses(Data &cmd)
{
	size_t i;
	int size;

	JDWMessage msg(acceptfd);

	// Build packet data
	Data response;

	size_t offset = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	// Size of known class list
	size = visibleClassList.size() - 1;

	AddJDWInt(response, offset, be_htobl(size));

	// Then, write the list of known class
	for (i=1; i<visibleClassList.size(); i++) {
		string str = visibleClassList[i].GetFullClassName();

		str = "L" + str + ";";
		replace_if(str.begin(), str.end(), bind2nd(equal_to<char>(),'.'), '/');

		AddJDWByte(response, offset, 0x01);
		AddJDWInt(response, offset, i);	// Should be equal to visibleClassList[i].index
		AddJDWString(response, offset, str);
		AddJDWInt(response, offset, be_htobl(0x04));
	}

	response.ReleaseBuffer(offset);


	size_t total_size = response.GetSize();

	// Fill in the header values
	MAKE_JDWPPACKETPTR_BUF(cpack, response.GetBuffer(total_size));
	Barry::Protocol::JDWP::Packet &packet = *cpack;


	MAKE_JDWPPACKET(rpack, cmd);

	packet.length = be_htobl(total_size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	response.ReleaseBuffer(total_size);
	msg.Send(response);
}


void JDWServer::CommandAllThreads(Data &cmd)
{
	JDWMessage msg(acceptfd);

	// Get threads list from device
	JVMThreadsList list;
	jvmdebug->GetThreadsList(list);
	dout(list);

	// Build packet data
	Data response;

	size_t offset = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	// Indicate the number of element
	AddJDWInt(response, offset, be_htobl(list.size()));

	// Send all threads ID
	JVMThreadsList::const_iterator b = list.begin();
	for( ; b != list.end(); b++ ) {
		const JVMThreadsEntry &entry = *b;

		AddJDWInt(response, offset, be_htobl(entry.Id));
	}

	response.ReleaseBuffer(offset);


	size_t total_size = response.GetSize();

	// Fill in the header values
	MAKE_JDWPPACKETPTR_BUF(cpack, response.GetBuffer(total_size));
	Barry::Protocol::JDWP::Packet &packet = *cpack;


	MAKE_JDWPPACKET(rpack, cmd);

	packet.length = be_htobl(total_size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	response.ReleaseBuffer(total_size);
	msg.Send(response);
}


void JDWServer::CommandIdSizes(Data &cmd)
{
	JDWMessage msg(acceptfd);

	MAKE_JDWPPACKET(rpack, cmd);

	size_t size;

	Barry::Protocol::JDWP::Packet packet;

	size = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE
		+ JDWP_PACKETVIRTUALMACHINEIDSIZES_DATA_SIZE;

	packet.length = be_htobl(size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);
	packet.u.response.u.virtualMachine.u.IDSizes.fieldIDSize = be_htobl(0x04);
	packet.u.response.u.virtualMachine.u.IDSizes.methodIDSize = be_htobl(0x04);
	packet.u.response.u.virtualMachine.u.IDSizes.objectIDSize = be_htobl(0x04);
	packet.u.response.u.virtualMachine.u.IDSizes.referenceTypeIDSize = be_htobl(0x04);
	packet.u.response.u.virtualMachine.u.IDSizes.frameIDSize = be_htobl(0x04);

	Data response(&packet, size);

	msg.Send(response);
}


void JDWServer::CommandSuspend(Data &cmd)
{
	JDWMessage msg(acceptfd);


	// Suspend device
	jvmdebug->Stop();

	// Notify debugger
	MAKE_JDWPPACKET(rpack, cmd);

	size_t size;

	Barry::Protocol::JDWP::Packet packet;

	size = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	packet.length = be_htobl(size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	Data response(&packet, size);

	msg.Send(response);
}


void JDWServer::CommandResume(Data &cmd)
{
	JDWMessage msg(acceptfd);


	// Resume device
	jvmdebug->Unknown06();
	jvmdebug->Unknown07();
	jvmdebug->Unknown08();
	jvmdebug->Unknown09();
	jvmdebug->Unknown10();
	jvmdebug->Go();

	// Notify debugger
	MAKE_JDWPPACKET(rpack, cmd);

	size_t size;

	Barry::Protocol::JDWP::Packet packet;

	size = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	packet.length = be_htobl(size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	Data response(&packet, size);

	msg.Send(response);
}


void JDWServer::CommandClassPaths(Data &cmd)
{
	JDWMessage msg(acceptfd);

	// Build packet data
	Data response;

	size_t offset = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE;

	AddJDWString(response, offset, string(""));
	AddJDWInt(response, offset, be_htobl(0));
	AddJDWInt(response, offset, be_htobl(0));

	response.ReleaseBuffer(offset);


	size_t total_size = response.GetSize();

	// Fill in the header values
	MAKE_JDWPPACKETPTR_BUF(cpack, response.GetBuffer(total_size));
	Barry::Protocol::JDWP::Packet &packet = *cpack;


	MAKE_JDWPPACKET(rpack, cmd);

	packet.length = be_htobl(total_size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);

	response.ReleaseBuffer(total_size);
	msg.Send(response);
}



void JDWServer::CommandSet(Data &cmd)
{
	static int value = 2;

	JDWMessage msg(acceptfd);

	MAKE_JDWPPACKET(rpack, cmd);

	size_t size;

	Barry::Protocol::JDWP::Packet packet;

	size = JDWP_PACKET_HEADER_SIZE + JDWP_RESPONSE_HEADER_SIZE + sizeof(uint32_t);

	packet.length = be_htobl(size);
	packet.id = rpack->id;
	packet.flags = 0x80;
	packet.u.response.errorcode = be_htobs(0);
	packet.u.response.u.value = be_htobl(value);

	Data response(&packet, size);

	msg.Send(response);

	value++;
}


}} // namespace Barry::JDWP

