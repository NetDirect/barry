///
/// \file	m_javadebug.cc
///		Mode class for the JavaDebug mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008-2009, Nicolas VIVIEN

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

#include "m_javadebug.h"
#include "data.h"
#include "protocol.h"
#include "protostructs.h"
#include "packet.h"
#include "endian.h"
#include "error.h"
#include "usbwrap.h"
#include "controller.h"
#include "cod.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "debug.h"

using namespace std;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// JDModulesList class

void JDModulesList::Parse(const Data &entry_packet)
{
	uint16_t count = 0;

	size_t size = entry_packet.GetSize();

	while (count < size) {
		uint16_t len = 0;
	
		const unsigned char *ptr = (entry_packet.GetData() + count);
		Protocol::JDModulesEntry *e = (Protocol::JDModulesEntry *) ptr;

		len = SB_JDMODULES_ENTRY_HEADER_SIZE + be_btohs(e->sizename);

		JDModulesEntry entry;

		entry.Id = be_btohl(e->id);
		entry.Address = be_btohl(e->address);
		(entry.Name).assign((char *) (ptr + SB_JDMODULES_ENTRY_HEADER_SIZE), be_btohs(e->sizename));

		push_back(entry);

		count += len;
	}
}


void JDModulesList::Dump(std::ostream &os) const
{
	const_iterator i = begin(), e = end();

	os << "     ID     " << "|";
	os << "   Address  " << "|";
	os << " Module Name" << endl;

	os << "------------+";
	os << "------------+";
	os << "-------------";
	os << endl;

	for( ; i != e; ++i ) {
		(*i).Dump(os);
	}
}


///////////////////////////////////////////////////////////////////////////////
// JDModulesEntry class
/*
void JDModulesEntry::Parse(uint16_t size, const char* buf)
{
	uint16_t len;
	const char *ptr = NULL;

	// Id of module
	Id = (uint32_t) be_btohl(*buf);

	// Address of module
	Address = (uint32_t) be_btohl(*(buf + 4));

	// Length of module name
	len = (uint16_t) be_btohs(*(buf + 8));

	// Name of module
	ptr = buf + 8;
	ptr += sizeof(uint16_t);

	Name.assign((char*)ptr, len);
}
*/
void JDModulesEntry::Dump(std::ostream &os) const
{
	os << " 0x" << setfill('0') << setw(8) << hex << Id << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << Address << " |";
	os << " " << Name << endl;
}


namespace Mode {

///////////////////////////////////////////////////////////////////////////////
// JavaDebug Mode class

JavaDebug::JavaDebug(Controller &con)
	: Mode(con, Controller::JavaDebug)
	, m_Attached(false)
{
}

JavaDebug::~JavaDebug()
{
	if( m_Attached )
		Detach();
}

///////////////////////////////////////////////////////////////////////////////
// protected members


///////////////////////////////////////////////////////////////////////////////
// public API

void JavaDebug::OnOpen()
{
	m_socket->InitSequence();
}

//
// Attach
//
/// These commands are sent to prepare the debug communication.
/// Must be called at the start of a JavaDebug session.
///
void JavaDebug::Attach()
{
}

//
// Detach
//
/// Must be called at the end of a JavaDebug session.  The JD_GOODBYE
/// command is sent to the device.
///
void JavaDebug::Detach()
{
}


void JavaDebug::ThrowJDError(const std::string &msg, uint8_t cmd)
{
	std::ostringstream oss;
	oss << msg << ": unexpected packet command code: 0x"
		<< std::hex << (unsigned int) cmd;
	throw Error(oss.str());
}


//
// Unknown 01 ???
//
void JavaDebug::Unknown01()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown01();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 02 ???
//
void JavaDebug::Unknown02()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown02();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 03 ???
//
void JavaDebug::Unknown03()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown03();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 04 ???
//
void JavaDebug::Unknown04()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown04();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 05 ???
//
void JavaDebug::Unknown05()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown05();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 06 ???
//
void JavaDebug::Unknown06()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown06();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 07 ???
//
void JavaDebug::Unknown07()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown07();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 08 ???
//
void JavaDebug::Unknown08()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown08();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 09 ???
//
void JavaDebug::Unknown09()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown09();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Unknown 10 ???
//
void JavaDebug::Unknown10()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Unknown10();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


//
// Get Status
//
void JavaDebug::GetStatus(int &status)
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.GetStatus();
	
	m_socket->Packet(packet);

	expect = packet.Size();

	while (expect == 0) {
		m_socket->Receive(packet.GetReceive());
	
		expect = packet.Size();
	}

	// Read the data stream
	m_socket->ReceiveData(response);

	MAKE_JDPACKET(dpack, response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::GetModulesList expect", expect);
	}

	// Return status
	status = dpack->u.status;
}


//
// Get Console Message
// Sample, display the output of System.out.println(...) and all JVM messages output
//
// Return the length message or -1 if message doesn't exit or is empty
//
int JavaDebug::GetConsoleMessage(std::string &message)
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.GetConsoleMessage();
	
	m_socket->Packet(packet);

	expect = packet.Size();

	if (expect == 0)
		return -1;

	// Read the data stream
	m_socket->ReceiveData(response);

	MAKE_JDPACKET(dpack, response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::GetModulesList expect", expect);
	}

	// Length of message
	uint16_t length = be_btohs(dpack->u.msglength);

	if (length == 0)
		return -1;

	// Parse the ID of nextmodules
	const unsigned char *ptr = (response.GetData() + SB_JDPACKET_HEADER_SIZE + sizeof(uint16_t));

	message.assign((char *) ptr, length);

	return length;
}


//
// Get list of Java modules
//
void JavaDebug::GetModulesList(JDModulesList &mylist) 
{
	uint32_t size = 0;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	do {
		// Send the command packet
		packet.GetModulesList(offset);
	
		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			break;

		// Read the data stream
		m_socket->ReceiveData(response);

		MAKE_JDPACKET(dpack, response);

		size_t bytereceived = response.GetSize() - 4;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJDError("JavaDebug::GetModulesList expect", expect);
		}

		// Number of modules entries in the list
		count = be_btohl(dpack->u.moduleslist.nbr);

		// Size of modules list
		// I remove the header of packet (contains the field 'number of modules')
		// and 4 bytes (contains the field 'ID of next modules')
		size = bytereceived - SB_JDMODULES_LIST_HEADER_SIZE - 4;

		// Parse the modules list
		mylist.Parse(Data(response.GetData() + SB_JDPACKET_HEADER_SIZE + SB_JDMODULES_LIST_HEADER_SIZE, size));

		// Parse the ID of nextmodules
		const unsigned char *ptr = (response.GetData() + SB_JDPACKET_HEADER_SIZE + SB_JDMODULES_LIST_HEADER_SIZE + size);
		uint32_t *poffset = (uint32_t *) ptr;

		offset = be_btohl(*poffset);
	} while (offset != 0);	// When the offset != 0, there is some modules
}


//
// Go
//
void JavaDebug::Go()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JDPacket packet(command, response);

	// Send the command packet
	packet.Go();
	m_socket->Packet(packet);
	expect = packet.Size();

	while (expect == 0) {
		m_socket->Receive(packet.GetReceive());
	
		expect = packet.Size();
	}

	// Read the data stream
	m_socket->ReceiveData(response);

	size_t bytereceived = response.GetSize() - 4;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJDError("JavaDebug::Attach expect", expect);
	}
}


}} // namespace Barry::Mode

