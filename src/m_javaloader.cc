///
/// \file	m_javaloader.cc
///		Mode class for the JavaLoader mode
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2008-2009, Nicolas VIVIEN

        Some parts are inspired from m_desktop.h

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

#include "m_javaloader.h"
#include "data.h"
#include "protocol.h"
#include "protostructs.h"
#include "packet.h"
#include "endian.h"
#include "error.h"
#include "usbwrap.h"
#include "controller.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <string.h>

#include "debug.h"

using namespace std;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// JLScreenInfo class

JLScreenInfo::JLScreenInfo()
{
}

JLScreenInfo::~JLScreenInfo()
{
}



///////////////////////////////////////////////////////////////////////////////
// JLDirectory class

JLDirectory::JLDirectory(int level)
	: m_level(level)
{
}

JLDirectory::~JLDirectory()
{
}

void JLDirectory::ParseTable(const Data &table_packet)
{
	m_idTable.clear();

	size_t count = table_packet.GetSize() / 2;
	uint16_t *item = (uint16_t*) table_packet.GetData();
	for( size_t i = 0; i < count; i++, item++ ) {
		m_idTable.push_back( be_btohs(*item) );
	}
}

void JLDirectory::Dump(std::ostream &os) const
{
	int indent = m_level * 2;

	os << setfill(' ') << setw(indent) << "";
	os << "Directory: " << m_idTable.size() << "/" << size() << " entries\n";

	const_iterator i = begin(), e = end();
	for( ; i != e; ++i ) {
		os << setfill(' ') << setw(indent + 2) << "";
		os << *i << "\n";
	}
}



///////////////////////////////////////////////////////////////////////////////
// JLDirectoryEntry class

JLDirectoryEntry::JLDirectoryEntry(int level)
	: m_level(level)
	, SubDir(level + 1)
{
}

void JLDirectoryEntry::Parse(uint16_t id, const Data &entry_packet)
{
	size_t needed = SB_JLDIRENTRY_HEADER_SIZE;
	size_t have = entry_packet.GetSize();
	if( have < needed )
		throw BadSize("JLDE:Parse(1)", have, needed);

	const unsigned char *ptr = entry_packet.GetData();
	Protocol::JLDirEntry *entry = (Protocol::JLDirEntry*) ptr;

	Id = id;
	Timestamp = be_btohl(entry->timestamp);

	uint16_t len = be_btohs(entry->filename_size);
	needed += len;
	if( have < needed )
		throw BadSize("JLDE:Parse(2)", have, needed);
	Name.assign((char *)entry->filename, len);

	// need parsed data + string size
	ptr += needed;
	needed += 2;
	if( have < needed )
		throw BadSize("JLDE:Parse(3)", have, needed);

	len = be_btohs( *((uint16_t*)(ptr)) );
	ptr += sizeof(uint16_t);
	needed += len;
	if( have < needed )
		throw BadSize("JLDE:Parse(4)", have, needed);
	Version.assign((char*)ptr, len);

	// need parsed data + string size
	ptr += len;
	needed += sizeof(uint32_t);
	if( have < needed )
		throw BadSize("JLDE:Parse(5)", have, needed);
	CodSize = be_btohl( *((uint32_t*)(ptr)) );
}

void JLDirectoryEntry::Dump(std::ostream &os) const
{
	os << left << setfill(' ') << setw(50) << Name;

	os << "\n";
	os << left << setw(28) << " ";

	os << "0x" << setfill('0') << setw(4) << hex << Id;
	os << " " << setw(10) << Version;
	os << " " << setw(7) << std::dec << CodSize;

	std::string ts = ctime(&Timestamp);
	ts.erase(ts.size() - 1);
	os << " " << ts;

	if( SubDir.size() )
		os << "\n" << SubDir;
}


namespace Mode {

///////////////////////////////////////////////////////////////////////////////
// JavaLoader Mode class

JavaLoader::JavaLoader(Controller &con)
	: m_con(con)
	, m_ModeSocket(0)
	, m_StreamStarted(false)
{
}

JavaLoader::~JavaLoader()
{
	if( m_StreamStarted )
		StopStream();
}

///////////////////////////////////////////////////////////////////////////////
// protected members


///////////////////////////////////////////////////////////////////////////////
// public API

//
// Open
//
/// Select device mode.  This is required before using any other mode-based
/// operations.
///
/// This function opens a socket to the device for communicating in Desktop
/// mode.  If the device requires it, specify the password with a const char*
/// string in password.  The password will not be stored in memory
/// inside this class, only a hash will be generated from it.  After
/// using the hash, the hash memory will be set to 0.  The application
/// is responsible for safely handling the raw password data.
///
/// You can retry the password by catching Barry::BadPassword and
/// calling RetryPassword() with the new password.
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	std::logic_error()
///		Thrown if unsupported mode is requested, or if socket
///		already open.
///
/// \exception	Barry::BadPassword
///		Thrown when password is invalid or if not enough retries
///		left in the device.
///
void JavaLoader::Open(const char *password)
{
	if( m_ModeSocket ) {
		m_socket->Close();
		m_socket.reset();
		m_ModeSocket = 0;
	}

	m_ModeSocket = m_con.SelectMode(Controller::JavaLoader);
	RetryPassword(password);
}

//
// RetryPassword
//
/// Retry a failed password attempt from the first call to Open().
/// Only call this function in response to Barry::BadPassword exceptions
/// that are thrown from Open().
///
/// \exception	Barry::Error
///		Thrown on protocol error.
///
/// \exception	std::logic_error()
///		Thrown if in unsupported mode, or if socket already open.
///
/// \exception	Barry::BadPassword
///		Thrown when password is invalid or if not enough retries
///		left in the device.
///
void JavaLoader::RetryPassword(const char *password)
{
	if( m_socket.get() != 0 )
		throw std::logic_error("Socket alreay open in RetryPassword");

	m_socket = m_con.m_zero.Open(m_ModeSocket, password);

	{
	Data response;
	m_socket->Receive(response, -1);
	}
}


// These commands are sent to prepare the data stream
void JavaLoader::StartStream()
{
	// 1°/
	char rawCommand1[] = { 4, 0, 0x08, 0, 0x64, 0, 0, 0 };
	*((uint16_t*) rawCommand1) = htobs(m_socket->GetSocket());

	Data command1(rawCommand1, sizeof(rawCommand1));
	Data response;

	try {
		m_socket->PacketData(command1, response);
	}
	catch( Usb::Error & ) {
		eout("JavaLoader: command1 error");
		eeout(command1, response);
		throw;
	}

	// 2°/
	char rawCommand2[] = { 4, 0, 0x08, 0, 0x70, 0, 0x01, 0 };
	*((uint16_t*) rawCommand2) = htobs(m_socket->GetSocket());

	Data command2(rawCommand2, sizeof(rawCommand2));

	try {
		m_socket->SetSequencePacket(false);
		m_socket->PacketData(command2, response);
		m_socket->SetSequencePacket(true);
	}
	catch( Usb::Error & ) {
		eout("JavaLoader: command2 error");
		eeout(command2, response);
		throw;
	}

	// 3°/
	char rawCommand3[] = { 4, 0, 0x05, 0, 0 };
	*((uint16_t*) rawCommand3) = htobs(m_socket->GetSocket());

	Data command3(rawCommand3, sizeof(rawCommand3));

	try {
		m_socket->PacketData(command3, response);
	}
	catch( Usb::Error & ) {
		eout("JavaLoader: command3 error");
		eeout(command1, response);
		throw;
	}

	m_StreamStarted = true;
}


// This function permits to send a COD application
// WARNING : Before, you have to call the "Start" function,
//           After, you have to call the "Stop" function.
//
// From the USB traces, the max size of packet is : 0x07FC
// Packet header :
//  04 00 08 00 68 00 F8 07
//                    ^^^^^ : about size
//              ^^ : command
//        ^^ : size of packet header
//  ^^^^^ : socket
// Response :
//  00 00 0C 00 13 04 01 00 0A 00 00 00
// Packet format : 
//  04 00 FC 07 DB 9D 95 2B 57 .... E6 FD
//              ^^^^^ ............. ^^^^^ : data (the file content)
//        ^^^^^ : packet size
//  ^^^^^ : socket
//
//
// WARNING : A COD file starts with the integer 0xDEC0FFFF (FIXME)
// An application can contain several COD parts. In this case we can read a header (start with PK)
// In this sample, we have to skip the file header :
//   00000000   50 4B 03 04  0A 00 00 00  00 00 A0 00  51 35 BA 9F  99 5D 30 CE  PK..........Q5...]0.
//   00000014   00 00 30 CE  00 00 15 00  04 00 4D 65  74 72 6F 56  69 65 77 65  ..0.......MetroViewe
//   00000028   72 2E 50 61  72 69 73 2E  63 6F 64 FE  CA 00 00 DE  C0 FF FF 00  r.Paris.cod.........
//                                                              ^^ Start of data sent !
//   0000003C   00 00 00 00  00 00 00 0F  10 34 45 00  00 00 00 00  00 00 00 21  .........4E........!
//   00000050   00 FF FF FF  FF FF FF FF  FF FF FF 4E  00 9C 08 68  C5 00 00 F0  ...........N...h....
//   00000064   B8 BC C0 A1  C0 14 00 81  00 00 01 01  04 0E 3F 6D  00 02 00 6D  ..............?m...m
void JavaLoader::SendStream(char *buffer, int buffsize)
{
	int bytesent = 0;

	unsigned char rawCommand6[] = { 4, 0, 0x08, 0, 0x68, 0, 0xf8, 0x07 };


	// 4°/
	char rawCommand4[] = { 4, 0, 0x08, 0, 0x67, 0x01, 0x04, 0 };
	*((uint16_t*) rawCommand4) = htobs(m_socket->GetSocket());

	Data command4(rawCommand4, sizeof(rawCommand4));
	Data response;

	try {
		m_socket->SetSequencePacket(false);
		m_socket->PacketData(command4, response);
		m_socket->SetSequencePacket(true);
	}
	catch( Usb::Error & ) {
		eout("JavaLoader: command4 error");
		eeout(command4, response);
		throw;
	}

	// 5°/
	char rawCommand5[] = { 4, 0, 0x08, 0, 0, 0, 0x00, 0x00 };
	*((uint16_t*) rawCommand5) = htobs(m_socket->GetSocket());
	*(((uint32_t*) rawCommand5) + 1) = be_htobl(buffsize);

	Data command5(rawCommand5, sizeof(rawCommand5));

	try {
		m_socket->PacketData(command5, response);
	}
	catch( Usb::Error & ) {
		eout("JavaLoader: command5 error");
		eeout(command5, response);
		throw;
	}


	// Read the buffer...
	while (bytesent < buffsize) {
		// Read data buffer
		int size;

		if (buffsize - bytesent > 0x7f8)
			size = 0x7f8;
		else
			size = buffsize - bytesent;

		char rawCommand7[0x7f8 + 4];
		memcpy(&rawCommand7[4], buffer, size);


		// 1st packet
		//------------
		// Packet Header
		*((uint16_t*) rawCommand6) = htobs(m_socket->GetSocket());
		*(((uint16_t*) rawCommand6) + 3) = htobs(size);

		Data command6(rawCommand6, sizeof(rawCommand6));

		try {
			m_socket->SetSequencePacket(false);
			m_socket->PacketData(command6, response);
			m_socket->SetSequencePacket(true);
		}
		catch( Usb::Error & ) {
			eout("JavaLoader: command6 error");
			eeout(command6, response);
			throw;
		}

		// 2nd packet
		//------------
		// Packet data
		*((uint16_t*) rawCommand7) = htobs(m_socket->GetSocket());
		*(((uint16_t*) rawCommand7) + 1) = htobs(size + 4);

		Data command7(rawCommand7, size + 4);

		try {
			m_socket->PacketData(command7, response);
		}
		catch( Usb::Error & ) {
			eout("JavaLoader: command7 error");
			eeout(command7, response);
			throw;
		}

		// Next...
		bytesent += size;
		buffer += size;
	}
}


// This command is sent to avert that the data stream is finished
void JavaLoader::StopStream(void)
{
	// 7°/
	unsigned char rawCommand[] = { 4, 0, 0x08, 0, 0x8d, 0, 0, 0 };
	*((uint16_t*) rawCommand) = htobs(m_socket->GetSocket());

	Data command(rawCommand, sizeof(rawCommand));
	Data response;
	m_socket->PacketData(command, response);

	m_StreamStarted = false;
}

void JavaLoader::SetTime(time_t when)
{
	Data cmd(-1, 8), data(-1, 8), response;

	JLPacket packet(cmd, data, response);
	packet.SetTime(when);
	m_socket->Packet(packet);
	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::SetTime", packet.Command());
	}
}

void JavaLoader::ThrowJLError(const std::string &msg, uint8_t cmd)
{
	std::ostringstream oss;
	oss << msg << ": unexpected packet command code: 0x"
		<< std::hex << (unsigned int) cmd;
	throw Error(oss.str());
}

void JavaLoader::GetDirectoryEntries(JLPacket &packet,
				     uint8_t entry_cmd,
				     JLDirectory &dir,
				     bool include_subdirs)
{
	JLDirectory::TableIterator i = dir.TableBegin(), e = dir.TableEnd();
	for( ; i != e; ++i ) {
		packet.GetDirEntry(entry_cmd, *i);
		m_socket->Packet(packet);
		if( packet.Command() != SB_COMMAND_JL_ACK ) {
			ThrowJLError("JavaLoader::GetDirectoryEntries", packet.Command());
		}

		Data &response = packet.GetReceive();
		m_socket->Receive(response);
		JLDirectoryEntry entry(dir.Level());
		Protocol::CheckSize(response, 4);
		entry.Parse(*i, Data(response.GetData() + 4, response.GetSize() - 4));

		if( include_subdirs ) {
			packet.GetSubDir(*i);
			GetDir(packet, SB_COMMAND_JL_GET_SUBDIR_ENTRY, entry.SubDir, false);
		}

		// add to list
		dir.push_back(entry);
	}
}

void JavaLoader::GetDir(JLPacket &packet,
			uint8_t entry_cmd,
			JLDirectory &dir,
			bool include_subdirs)
{
	m_socket->Packet(packet);
	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::GetDir", packet.Command());
	}

	Data &response = packet.GetReceive();
	m_socket->Receive(response);
	Protocol::CheckSize(response, 4);
	dir.ParseTable(Data(response.GetData() + 4, response.GetSize() - 4));
	GetDirectoryEntries(packet, entry_cmd, dir, include_subdirs);
}

void JavaLoader::GetDirectory(JLDirectory &dir, bool include_subdirs)
{
	Data cmd(-1, 8), data(-1, 8), response;
	JLPacket packet(cmd, data, response);

	packet.GetDirectory();
	GetDir(packet, SB_COMMAND_JL_GET_DATA_ENTRY, dir, include_subdirs);
}


// This function permits to receive a ScreenShot (maybe other...)
// WARNING : Before, you have to call the "Start" function,
//           After, you have to call the "Stop" function.
//
// From the USB traces, the max size of packet is : 0x07FC
// When you are ready, we send the packet :
//  04 00 08 00 68 00 00 00
// Then, we receive an acknoledge and the data.
// The data is composed of two packets : header and content.
// Packet header :
//  04 00 08 00 6E 00 F8 07
//                    ^^^^^ : size + 4 bytes
//              ^^ : command
//        ^^^^^ : size of packet header
//  ^^^^^ : socket
// Packet content : 
//  04 00 FC 07 DB 9D 95 2B 57 .... E6 FD
//              ^^^^^ ............. ^^^^^ : data (the file content)
//        ^^^^^ : packet size (0x07FC = 0x7F8 + 4)
//  ^^^^^ : socket
//
char * JavaLoader::GetScreenshot(JLScreenInfo &info, char *buffer, size_t *buffsize)
{
	size_t expect = 0;
	size_t bytereceived = 0;

	Data cmd(-1, 8), data(-1, 8), response;
	JLPacket packet(cmd, data, response);

	*buffsize = 0;

	// Send the screenshot command :
	//    00000000: 04 00 08 00 87 00 04 00
	packet.GetScreenshot();

	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::GetScreenshot", packet.Command());
	}

	// Get Info :
	//    00000000: 04 00 14 00 00 05 46 00 40 03 01 68 01 e0 00 10  ......F.@..h....
	//                                            ^^^^^x^^^^^ : width x height
	//                    ^^^^^ : packet size
	//              ^^^^^ : socket ID
	//    00000010: 00 00 00 00                                      ....

	response = packet.GetReceive();
	m_socket->Receive(response);
	Protocol::CheckSize(response, 4);

	// Parse response...
	MAKE_JLPACKET(rpack, response);
	
	info.width = be_btohs(rpack->u.screeninfo.width);
	info.height = be_btohs(rpack->u.screeninfo.height);


	// Read stream
	for (;;) {
		// Send the packet :
		//   04 00 08 00 68 00 00 00
		packet.GetData();

		m_socket->Packet(packet);

		// Read and parse the response
		//   04 00 08 00 64 00 00 00 
		// or 
		//   04 00 08 00 6e 00 f8 07

		if( packet.Command() == SB_COMMAND_JL_ACK )
			return buffer;

		if( packet.Command() != SB_COMMAND_JL_GET_DATA_ENTRY ) {
			ThrowJLError("JavaLoader::GetScreenShot ", packet.Command());
		}

		// Read the size of next packet
		expect = packet.Size();


		// Read the stream
		response = packet.GetReceive();
		m_socket->Receive(response);
		Protocol::CheckSize(response, 4);


		// Save data in buffer
		const unsigned char *pd = response.GetData();
		bytereceived = (size_t) response.GetSize() - 4;


		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJLError("JavaLoader::GetScreenShot expect", expect);
		}


		// Copy data
		buffer = (char *) realloc(buffer, (*buffsize) + bytereceived);

		memcpy(buffer + (*buffsize), pd + 4, bytereceived);

		// New size
		(*buffsize) += bytereceived;
	}

	return 0;
}


}} // namespace Barry::Mode

