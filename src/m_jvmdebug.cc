///
/// \file	m_jvmdebug.cc
///		Mode class for the JVMDebug mode
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
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

#include "i18n.h"
#include "m_jvmdebug.h"
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
#include "ios_state.h"

#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// JDModulesList class

void JVMModulesList::Parse(const Data &entry_packet)
{
	uint32_t count = 0;

	size_t size = entry_packet.GetSize();

	while (count < size) {
		uint16_t len = 0;

		const unsigned char *ptr = (entry_packet.GetData() + count);
		Protocol::JVMModulesEntry *e = (Protocol::JVMModulesEntry *) ptr;

		len = SB_JVMMODULES_ENTRY_HEADER_SIZE + be_btohs(e->sizename);
		if( (count + len) > size )
			break;

		JVMModulesEntry entry;

		entry.Id = be_btohl(e->id);
		entry.UniqueID = be_btohl(e->uniqueId);
		(entry.Name).assign((char *) (ptr + SB_JVMMODULES_ENTRY_HEADER_SIZE), be_btohs(e->sizename));

		push_back(entry);

		count += len;
	}
}


void JVMModulesList::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	const_iterator i = begin(), e = end();

	os << _("     ID     ") << "|";
	os << _("  UniqueID  ") << "|";
	os << _(" Module Name") << endl;

	os << "------------+";
	os << "------------+";
	os << "-------------";
	os << endl;

	for( ; i != e; ++i ) {
		(*i).Dump(os);
	}
}


///////////////////////////////////////////////////////////////////////////////
// JVMModulesEntry class

void JVMModulesEntry::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os << " 0x" << setfill('0') << setw(8) << hex << Id << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << UniqueID << " |";
	os << " " << Name << endl;
}


///////////////////////////////////////////////////////////////////////////////
// JVMThreadsList class

void JVMThreadsList::Parse(const Data &entry_packet)
{
	uint32_t count = 0;

	size_t size = entry_packet.GetSize();

	while (count < size) {
		uint16_t len = 0;

		const unsigned char *ptr = (entry_packet.GetData() + count);
		uint32_t *e = (uint32_t *) ptr;

		len = sizeof(uint32_t);
		if( (count + len) > size )
			break;

		JVMThreadsEntry entry;

		entry.Id = be_btohl(*e);

		push_back(entry);

		count += len;
	}
}


void JVMThreadsList::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	const_iterator i = begin(), e = end();

	os << _("  Thread  ") << "|";
	os << _("  Address   ") << "|";
	os << _(" Byte ") << "|";
	os << _(" Unknown01  ") << "|";
	os << _(" Unknown02  ") << "|";
	os << _(" Unknown03  ") << "|";
	os << _(" Unknown04  ") << "|";
	os << _(" Unknown05  ") << "|";
	os << _(" Unknown06  ") << "|";

	os << "------------+";
	os << "------------+";
	os << "------+";
	os << "------------+";
	os << "------------+";
	os << "------------+";
	os << "------------+";
	os << "------------+";
	os << "-------------";
	os << endl;

	for(int k=0 ; i != e; ++i, k++ ) {
		(*i).Dump(os, k);
	}
}


void JVMThreadsEntry::Dump(std::ostream &os, int num) const
{
	ios_format_state state(os);

	os << " " << setfill(' ') << setw(8) << dec << num << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Id) << " |";
	os << " 0x" << setfill('0') << setw(2) << hex << (Byte) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Address) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown01) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown02) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown03) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown04) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown05) << " |";
	os << " 0x" << setfill('0') << setw(8) << hex << (Unknown06) << endl;
}


namespace Mode {

///////////////////////////////////////////////////////////////////////////////
// JVMDebug Mode class

JVMDebug::JVMDebug(Controller &con)
	: Mode(con, Controller::JVMDebug)
	, m_Attached(false)
{
}

JVMDebug::~JVMDebug()
{
	if( m_Attached )
		Detach();
}

///////////////////////////////////////////////////////////////////////////////
// protected members


///////////////////////////////////////////////////////////////////////////////
// public API

void JVMDebug::OnOpen()
{
}

// FIXME - is this necessary?  and if it is, wouldn't it be better
// in the m_jvmdebug mode class?  I'm not convinced that applications
// should have to bother with socket-level details.
void JVMDebug::Close()
{
	if( m_ModeSocket ) {
		m_socket->Close();
		m_socket.reset();
		m_ModeSocket = 0;
	}
}

//
// Attach
//
/// These commands are sent to prepare the debug communication.
/// Must be called at the start of a JVMDebug session.
///
void JVMDebug::Attach()
{
}

//
// Detach
//
/// Must be called at the end of a JVMDebug session.  The JVM_GOODBYE
/// command is sent to the device.
///
void JVMDebug::Detach()
{
}


void JVMDebug::ThrowJVMError(const std::string &msg, uint16_t cmd)
{
	std::ostringstream oss;
	oss << msg << ": " << _("unexpected packet command code: ")
		<< "0x" << std::hex << (unsigned int) cmd;
	throw Error(oss.str());
}


//
// Unknown 01 ???
//
void JVMDebug::Unknown01()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown01();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown01(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 02 ???
//
void JVMDebug::Unknown02()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown02();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown02(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 03 ???
//
void JVMDebug::Unknown03()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown03();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown03(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 04 ???
//
void JVMDebug::Unknown04()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown04();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown04(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 05 ???
//
void JVMDebug::Unknown05()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown05();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown05(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 06 ???
//
void JVMDebug::Unknown06()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown06();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown06(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 07 ???
//
void JVMDebug::Unknown07()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown07();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown07(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 08 ???
//
void JVMDebug::Unknown08()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown08();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown08(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 09 ???
//
void JVMDebug::Unknown09()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown09();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown09(): ") + _("byte count mismatch"), expect);
	}
}


//
// Unknown 10 ???
//
void JVMDebug::Unknown10()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Unknown10();
	m_socket->Packet(packet);
	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Unknown10(): ") + _("byte count mismatch"), expect);
	}
}


//
// Get Status
//
bool JVMDebug::GetStatus(int &status)
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.GetStatus();

	m_socket->Packet(packet);

	expect = packet.Size();

	if (expect == 0)
		return false;

	// Read the data stream
	m_socket->Receive(response);

	MAKE_JVMPACKET(dpack, response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::GetStatus():") + _("byte count mismatch"), expect);
	}

	// Make sure we have a header to read
	CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.status));

	// Return status
	status = dpack->u.status;

	return true;
}


//
// Wait Status
//
bool JVMDebug::WaitStatus(int &status)
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Prepare the command packet
	packet.GetStatus();

	try {
		m_socket->Receive(packet.GetReceive(), 100);
	} catch (Usb::Timeout &DEBUG_ONLY(to) ) {
		return false;
	}

	expect = packet.Size();

	if (expect == 0)
		return false;

	// Read the data stream
	m_socket->Receive(response);

	MAKE_JVMPACKET(dpack, response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::WaitStatus():") + _("byte count mismatch"), expect);
	}

	// Make sure we have a header to read
	CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.status));

	// Return status
	status = dpack->u.status;

	return true;
}


//
// Get Console Message
// Sample, display the output of System.out.println(...) and all JVM messages output
//
// Return the length message or -1 if message doesn't exit or is empty
//
int JVMDebug::GetConsoleMessage(std::string &message)
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.GetConsoleMessage();

	m_socket->Packet(packet);

	expect = packet.Size();

	if (expect == 0)
		return -1;

	// Read the data stream
	m_socket->Receive(response);

	MAKE_JVMPACKET(dpack, response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::GetConsoleMessage():") + _("byte count mismatch"), expect);
	}

	// Make sure we have a header to read
	CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.msglength));

	// Length of message
	uint16_t length = be_btohs(dpack->u.msglength);

	if (length == 0)
		return -1;

	CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.msglength) + length);

	// Parse the ID of nextmodules
	const unsigned char *ptr = (response.GetData() + SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.msglength));

	message.assign((char *) ptr, length);

	return length;
}


//
// Get list of Java modules
//
void JVMDebug::GetModulesList(JVMModulesList &mylist)
{
	uint32_t size = 0;
	uint32_t offset = 0;
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	do {
		// Send the command packet
		packet.GetModulesList(offset);

		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			break;

		// Read the data stream
		m_socket->Receive(response);

//		MAKE_JVMPACKET(dpack, response);	// unused

		size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJVMError(string("JVMDebug::GetModulesList():") + _("byte count mismatch"), expect);
		}

		// Make sure there's enough for packet header + module list
		// header + 4 bytes of ID
		CheckSize(response, SB_JVMPACKET_HEADER_SIZE + SB_JVMMODULES_LIST_HEADER_SIZE + 4);

		// Number of modules entries in the list
		// (Valid, but unused variable... disabled to stop compiler
		// warnings)
//		uint32_t count = be_btohl(dpack->u.moduleslist.nbr);

		// Size of modules list
		// I remove the header of packet (contains the field 'number of modules')
		// and 4 bytes (contains the field 'ID of next modules')
		size = bytereceived - SB_JVMMODULES_LIST_HEADER_SIZE - 4;

		// Parse the modules list
		mylist.Parse(Data(response.GetData() + SB_JVMPACKET_HEADER_SIZE + SB_JVMMODULES_LIST_HEADER_SIZE, size));

		// Parse the ID of nextmodules
		size_t id_offset = SB_JVMPACKET_HEADER_SIZE + SB_JVMMODULES_LIST_HEADER_SIZE + size;
		const unsigned char *ptr = (response.GetData() + id_offset);
		CheckSize(response, id_offset + sizeof(uint32_t));
		uint32_t *poffset = (uint32_t *) ptr;

		offset = be_btohl(*poffset);
	} while (offset != 0);	// When the offset != 0, there is some modules
}


//
// Get list of Java threads
//
void JVMDebug::GetThreadsList(JVMThreadsList &mylist)
{
	uint32_t size = 0;
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.GetThreadsList();

	m_socket->Packet(packet);

	expect = packet.Size();

	if (expect == 0)
		return;

	// Read the data stream
	m_socket->Receive(response);

//	MAKE_JVMPACKET(dpack, response);	// unused

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::GetThreadsList():") + _("byte count mismatch"), expect);
	}

	CheckSize(response, SB_JVMPACKET_HEADER_SIZE + SB_JVMTHREADS_LIST_HEADER_SIZE);

	// Number of threads entries in the list
	// (Valid, but unused variable... disabled to stop compiler warnings)
//	uint32_t count = be_btohl(dpack->u.threadslist.nbr);

	// Size of threads list
	// I remove the header of packet (contains the field 'number of threads')
	size = bytereceived - SB_JVMTHREADS_LIST_HEADER_SIZE;

	// Parse the threads list
	mylist.Parse(Data(response.GetData() + SB_JVMPACKET_HEADER_SIZE + SB_JVMTHREADS_LIST_HEADER_SIZE, size));

	// Complete threads list
	JVMThreadsList::iterator b = mylist.begin();
	for( ; b != mylist.end(); b++ ) {
		JVMThreadsEntry entry = (*b);

		// 1°/
		// Send the command packet
		packet.Unknown11(entry.Id);

		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			return;

		// Read the data stream
		m_socket->Receive(response);

		MAKE_JVMPACKET(dpack, response);

		bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJVMError(string("JVMDebug::GetThreadsList() (1):") + _("byte count mismatch"), expect);
		}

		CheckSize(response, SB_JVMPACKET_HEADER_SIZE + SB_JVMUNKNOWN01_HEADER_SIZE);

		// Save values
		entry.Byte = dpack->u.unknown01.byte;
		entry.Address = be_btohl(dpack->u.unknown01.address);

		// 2°/
		if (entry.Address != 0) {
			// Send the command packet
			packet.Unknown12(entry.Address);

			m_socket->Packet(packet);

			expect = packet.Size();

			if (expect == 0)
				return;

			// Read the data stream
			m_socket->Receive(response);

			MAKE_JVMPACKET(dpack, response);

			bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

			// Check the size read into the previous packet
			if( expect != bytereceived ) {
				ThrowJVMError(string("JVMDebug::GetThreadsList() (2):") + _("byte count mismatch"), expect);
			}


			// Save values
			CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.address));
			entry.Unknown01 = be_btohl(dpack->u.address);
		}
		else
			entry.Unknown01 = 0;

		// 3°/
		// Send the command packet
		packet.Unknown13(entry.Id);

		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			return;

		// Read the data stream
		m_socket->Receive(response);

		dpack = (const Protocol::JVMPacket *) response.GetData();

		bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJVMError(string("JVMDebug::GetThreadsList() (3):") + _("byte count mismatch"), expect);
		}

		// Save values
		CheckSize(response, SB_JVMPACKET_HEADER_SIZE + sizeof(dpack->u.address));
		entry.Unknown02 = be_btohl(dpack->u.address);

		// 4°/
		// Send the command packet
		packet.Unknown14(entry.Id);

		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			return;

		// Read the data stream
		m_socket->Receive(response);

		dpack = (const Protocol::JVMPacket *) response.GetData();

		bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJVMError(string("JVMDebug::GetThreadsList() (4):") + _("byte count mismatch"), expect);
		}

		// Save values
		CheckSize(response, SB_JVMPACKET_HEADER_SIZE + SB_JVMUNKNOWN02_HEADER_SIZE);
		entry.Unknown03 = be_btohl(dpack->u.unknown02.address1);
		entry.Unknown04 = be_btohl(dpack->u.unknown02.address2);

		// 5°/
		// Send the command packet
		packet.Unknown15(entry.Id);

		m_socket->Packet(packet);

		expect = packet.Size();

		if (expect == 0)
			return;

		// Read the data stream
		m_socket->Receive(response);

		dpack = (const Protocol::JVMPacket *) response.GetData();

		bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJVMError(string("JVMDebug::GetThreadsList() (5):") + _("byte count mismatch"), expect);
		}

		// Save values
		CheckSize(response, SB_JVMPACKET_HEADER_SIZE + SB_JVMUNKNOWN02_HEADER_SIZE);
		entry.Unknown05 = be_btohl(dpack->u.unknown02.address1);
		entry.Unknown06 = be_btohl(dpack->u.unknown02.address2);

		// Save...
		(*b) = entry;
	}
}


//
// Go
//
void JVMDebug::Go()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Go();
	m_socket->Packet(packet);
	expect = packet.Size();

	while (expect == 0) {
		m_socket->Receive(packet.GetReceive());

		expect = packet.Size();
	}

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Go():") + _("byte count mismatch"), expect);
	}
}


//
// Stop
//
void JVMDebug::Stop()
{
	uint16_t expect = 0;

	Data command(-1, 8), response;
	JVMPacket packet(command, response);

	// Send the command packet
	packet.Stop();
	m_socket->Packet(packet);
	expect = packet.Size();

	while (expect == 0) {
		m_socket->Receive(packet.GetReceive());

		expect = packet.Size();
	}

	// Read the data stream
	m_socket->Receive(response);

	size_t bytereceived = response.GetSize() - SB_JVMPACKET_HEADER_SIZE;

	// Check the size read into the previous packet
	if( expect != bytereceived ) {
		ThrowJVMError(string("JVMDebug::Stop():") + _("byte count mismatch"), expect);
	}
}


}} // namespace Barry::Mode

