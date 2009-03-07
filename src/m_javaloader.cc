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


///////////////////////////////////////////////////////////////////////////////
// JLEventlog class

void JLEventlog::Dump(std::ostream &os) const
{
	const_iterator i = begin(), e = end();
	for( ; i != e; ++i ) {
		(*i).Dump(os);
	}
}


///////////////////////////////////////////////////////////////////////////////
// JLEventlogEntry class, static members

//
// Note! These functions currently only pass the same values through.
//       In actuality, these are technically two different values:
//       one on the raw protocol side, and the other part of the
//       guaranteed Barry API.  If the Blackberry ever changes the
//       meanings for these codes, do the translation here.
//

JLEventlogEntry::Severity_t JLEventlogEntry::SeverityProto2Rec(unsigned int s)
{
	return (Severity_t)s;
}

unsigned int JLEventlogEntry::SeverityRec2Proto(Severity_t s)
{
	return s;
}

JLEventlogEntry::ViewerType_t JLEventlogEntry::ViewerTypeProto2Rec(unsigned int v)
{
	return (ViewerType_t)v;
}

unsigned int JLEventlogEntry::ViewerTypeRec2Proto(ViewerType_t v)
{
	return v;
}


///////////////////////////////////////////////////////////////////////////////
// JLEventlogEntry class

void JLEventlogEntry::Parse(uint16_t size, const char* buf)
{
	// example of a single log entry
	//guid:92E11214401C3 time:0x11F133E6470 severity:0 type:2 app:UI data:GS-D 2c89868b

	std::string src = std::string(buf, size);
	std::istringstream ss(src);

	ss.ignore(5); // skip "guid:"
	ss >> Guid;
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad guid field");

	ss.ignore(6); // skip " time:"
	ss >> hex >> MSTimestamp;
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad time field");

	ss.ignore(10); // skip " severity:"
	unsigned int severity;
	ss >> severity;
	Severity = SeverityProto2Rec(severity);
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad severity field");

	ss.ignore(6); // skip " type:"
	unsigned int type;
	ss >> type;
	Type = ViewerTypeProto2Rec(type);
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad type field");

	ss.ignore(5); // skip " app:"
	ss >> App;
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad app field");

	ss.ignore(6); // skip " data:"

	// use stringbuf to extract rest of data from stream
	stringbuf databuf;
	ss >> &databuf;
	if( ss.fail() )
		throw BadData("JLEventlogEntry:Parse bad data field");

	Data = databuf.str();
}

std::string JLEventlogEntry::GetFormattedTimestamp() const
{
	char buf[21];
	struct tm split;
	time_t timestamp = (time_t) (MSTimestamp / 1000);

	if( localtime_r(&timestamp, &split) == NULL )
		return "";

	if( strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S.", &split) == 0 )
		return "";

	std::ostringstream oss;
	oss << buf << (MSTimestamp % 1000);
	return oss.str();
}

void JLEventlogEntry::Dump(std::ostream &os) const
{
	static const char *SeverityNames[] = { "Always Log", "Severe Error", "Error",
		"Warning", "Information", "Debug Info"};
	static const char *ViewerTypes[] = { "", "Number", "String", "Exception" };

	os << "guid:"      << Guid;
	os << " time:"     << GetFormattedTimestamp();
	os << " severity:" << SeverityNames[Severity];
	os << " type:"     << ViewerTypes[Type];
	os << " app:"      << App;
	os << " data:"     << Data << endl;
}


///////////////////////////////////////////////////////////////////////////////
// JLDeviceInfo class

void JLDeviceInfo::Dump(std::ostream &os) const
{
	os << left << setfill(' ') << setw(17) << "Hardware Id:";
	os << "0x" << hex << HardwareId << endl;

	os << left << setfill(' ') << setw(17) << "PIN:";
	os << "0x" << hex << Pin << endl;

	os << left << setfill(' ') << setw(17) << "OS Version:";
	os << dec << OsVersion.Major << '.' << OsVersion.Minor << '.' << OsVersion.SubMinor << '.' << OsVersion.Build << endl;

	os << left << setfill(' ') << setw(17) << "VM Version:";
	os << dec << VmVersion.Major << '.' << VmVersion.Minor << '.' << VmVersion.SubMinor << '.' << VmVersion.Build << endl;

	os << left << setfill(' ') << setw(17) << "Radio ID:";
	os << "0x" << hex << RadioId << endl;

	os << left << setfill(' ') << setw(17) << "Vendor ID:";
	os << dec << VendorId << endl;

	os << left << setfill(' ') << setw(17) << "Active WAFs:";
	os << "0x" << hex << ActiveWafs << endl;

	os << left << setfill(' ') << setw(17) << "OS Metrics:" << endl;
	os << OsMetrics;

	os << left << setfill(' ') << setw(17) << "Bootrom Metrics:" << endl;
	os << BootromMetrics;
}


namespace Mode {

///////////////////////////////////////////////////////////////////////////////
// JavaLoader Mode class

JavaLoader::JavaLoader(Controller &con)
	: Mode(con, Controller::JavaLoader)
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

void JavaLoader::OnOpen()
{
	Data response;
	m_socket->Receive(response, -1);
}

// These commands are sent to prepare the data stream
void JavaLoader::StartStream()
{
	Data cmd(-1, 8), data(-1, 8), response;
	JLPacket packet(cmd, data, response);

	packet.Hello();
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_HELLO_ACK ) {
		ThrowJLError("JavaLoader::StartStream Hello", packet.Command());
	}

	packet.SetUnknown1();
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::StartStream Unknown1", packet.Command());
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
void JavaLoader::SendStream(std::istream &input, size_t module_size)
{
	char buffer[MAX_PACKET_DATA_SIZE - SB_JLPACKET_HEADER_SIZE];
	size_t max_data_size = sizeof(buffer);

	size_t remaining = module_size;

	Data cmd(-1, 8), data(-1, 8), response;
	JLPacket packet(cmd, data, response);

	packet.SetCodSize(module_size);
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::SendStream set code size", packet.Command());
	}

	while( remaining > 0 ) {
		size_t size = min(remaining, max_data_size);

		input.read(buffer, size);
		if( input.fail() || (size_t)input.gcount() != size ) {
			throw Error("JavaLoader::SendStream input stream read failed");
		}

		packet.PutData(buffer, size);
		m_socket->Packet(packet);

		if( packet.Command() != SB_COMMAND_JL_ACK ) {
			ThrowJLError("JavaLoader::SendStream send data", packet.Command());
		}

		remaining -= size;
	}
}

void JavaLoader::LoadApp(std::istream &input)
{
	uint32_t module_size;
	while( (module_size = SeekNextCod(input)) != 0 ) {
		SendStream(input, module_size);
	}
}

//
// StopStream
//
/// Must be called at the end of a JavaLoader session.  The JL_GOODBYE
/// command is sent to the device.  When the device responds with
/// RESET_REQUIRED the device reset command will be sent when the
/// socket is closed.
///
/// \return true when a device reset was required
///
bool JavaLoader::StopStream()
{
	Data cmd(-1, 8), data(-1, 8), response;

	JLPacket packet(cmd, data, response);
	packet.Goodbye();
	m_socket->Packet(packet);

	m_StreamStarted = false;

	if( packet.Command() == SB_COMMAND_JL_RESET_REQUIRED ) {
		m_con.m_zero.SetResetOnClose(true);
		return true;
	}
	else if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::StopStream", packet.Command());
	}

	return false;
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

	// ack response will contain length of module ID array in next packet
	unsigned int expect = packet.Size();

	if( expect > 0 ) {
		Data &response = packet.GetReceive();
		m_socket->Receive(response);
		Protocol::CheckSize(response, 4);
		dir.ParseTable(Data(response.GetData() + 4, response.GetSize() - 4));
		GetDirectoryEntries(packet, entry_cmd, dir, include_subdirs);
	}
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
//
// GetScreenshot
//
/// Downloads screenshot from device, and fills info with size data
/// and the given Data buffer image with the bitmap.
///
void JavaLoader::GetScreenshot(JLScreenInfo &info, Data &image)
{
	// start fresh
	image.Zap();

	Data cmd(-1, 8), data(-1, 8), response;
	JLPacket packet(cmd, data, response);

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

	m_socket->Receive(response);

	// Parse response...
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + SB_JLSCREENINFO_SIZE);
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
			return;

		if( packet.Command() != SB_COMMAND_JL_GET_DATA_ENTRY ) {
			ThrowJLError("JavaLoader::GetScreenShot ", packet.Command());
		}

		// Read the size of next packet
		size_t expect = packet.Size();


		// Read the stream
		m_socket->Receive(response);


		// Save data in buffer
		Protocol::CheckSize(response, 4);
		const unsigned char *pd = response.GetData();
		size_t bytereceived = response.GetSize() - 4;


		// Check the size read into the previous packet
		if( expect != bytereceived ) {
			ThrowJLError("JavaLoader::GetScreenShot expect", expect);
		}


		// Copy data
		unsigned char *buffer = image.GetBuffer(image.GetSize() + bytereceived);
		memcpy(buffer + image.GetSize(), pd + 4, bytereceived);

		// New size
		image.ReleaseBuffer(image.GetSize() + bytereceived);
	}
}

void JavaLoader::DoErase(uint8_t cmd, const std::string &cod_name)
{
	Data command(-1, 8), data(-1, 8), response;

	JLPacket packet(command, data, response);

	// set filename, device responds with an ID
	packet.SetCodFilename(cod_name);
	m_socket->Packet(packet);
	if( packet.Command() == SB_COMMAND_JL_COD_NOT_FOUND ) {
		throw Error(string("JavaLoader::DoErase: module ") + cod_name + " not found");
	}
	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::DoErase", packet.Command());
	}

	// make sure there is an ID coming
	if( packet.Size() != 2 )
		throw Error("JavaLoader::DoErase: expected code not available");

	// get ID
	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + sizeof(uint16_t));
	MAKE_JLPACKET(jpack, response);
	uint16_t id = be_btohs(jpack->u.id);

	// send erase command, with application ID
	packet.Erase(cmd, id);
	m_socket->Packet(packet);
	if( packet.Command() == SB_COMMAND_JL_COD_IN_USE ) {
		throw Error("JavaLoader::DoErase: COD file in use.");
	}
	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::DoErase", packet.Command());
	}
}

void JavaLoader::Erase(const std::string &cod_name)
{
	DoErase(SB_COMMAND_JL_ERASE, cod_name);
}

void JavaLoader::ForceErase(const std::string &cod_name)
{
	DoErase(SB_COMMAND_JL_FORCE_ERASE, cod_name);
}

void JavaLoader::GetEventlog(JLEventlog &log)
{
	Data command(-1, 8), data(-1, 8), response;
	JLPacket packet(command, data, response);

	packet.GetEventlog();

	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::GetEventlog", packet.Command());
	}

	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + sizeof(uint16_t));

	// number of eventlog entries
	MAKE_JLPACKET(jpack, response);
	uint16_t count = be_btohs(jpack->u.response.expect);

	for( uint16_t i = 0; i < count; ++ i ) {
		packet.GetEventlogEntry(i);

		m_socket->Packet(packet);

		if( packet.Command() != SB_COMMAND_JL_ACK ) {
			ThrowJLError("JavaLoader::GetEventlog", packet.Command());
		}

		m_socket->Receive(response);
		Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + SB_JLEVENTLOG_ENTRY_HEADER_SIZE);

		MAKE_JLPACKET(jpack, response);
		uint16_t size = be_btohs(jpack->u.logentry.size);

		JLEventlogEntry entry;
		entry.Parse(size, (const char *)(response.GetData() + SB_JLPACKET_HEADER_SIZE + SB_JLEVENTLOG_ENTRY_HEADER_SIZE));

		log.push_back(entry);
	}
}

void JavaLoader::ClearEventlog()
{
	Data command(-1, 8), data(-1, 8), response;
	JLPacket packet(command, data, response);

	packet.ClearEventlog();
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::ClearEventlog", packet.Command());
	}
}

void JavaLoader::SaveData(JLPacket &packet, uint16_t id, CodFileBuilder &builder, std::ostream &output)
{
	packet.SaveModule(id);
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::SaveData", packet.Command());
	}

	// get total size of cod file or this sibling cod file
	Data &response = packet.GetReceive();
	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + sizeof(uint32_t));
	MAKE_JLPACKET(jpack, response);
	uint32_t total_size = be_btohl(jpack->u.cod_size);

	// allocate buffer to hold data for this sibling
	Data buffer(-1, total_size);
	uint32_t offset = 0;

	for( ;; ) {
		packet.GetData();
		m_socket->Packet(packet);

		if( packet.Command() == SB_COMMAND_JL_ACK )
			break;

		if( packet.Command() != SB_COMMAND_JL_GET_DATA_ENTRY ) {
			ThrowJLError("JavaLoader::SaveData", packet.Command());
		}

		// expected size of data in response packet
		unsigned int expect = packet.Size();

		m_socket->Receive(response);
		Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + expect);

		memcpy(buffer.GetBuffer(offset + expect) + offset,
			response.GetData() + SB_JLPACKET_HEADER_SIZE,
			expect);

		offset += expect;
	}

	buffer.ReleaseBuffer(offset);

	builder.WriteNextHeader(output, buffer.GetData(), buffer.GetSize());
	output.write((const char *)buffer.GetData(), buffer.GetSize());
}

void JavaLoader::Save(const std::string &cod_name, std::ostream &output)
{
	Data command(-1, 8), data(-1, 8), response;

	JLPacket packet(command, data, response);

	// set filename, device responds with an ID
	packet.SetCodFilename(cod_name);
	m_socket->Packet(packet);

	if( packet.Command() == SB_COMMAND_JL_COD_NOT_FOUND ) {
		throw Error(string("JavaLoader::Save: module ") + cod_name + " not found");
	}

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::Save", packet.Command());
	}

	// make sure there is an ID coming
	if( packet.Size() != 2 )
		throw Error("JavaLoader::Save: expected module ID");

	// get ID
	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + sizeof(uint16_t));
	MAKE_JLPACKET(jpack, response);
	uint16_t id = be_btohs(jpack->u.id);

	// get list of sibling modules
	packet.GetSubDir(id);
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::Save", packet.Command());
	}

	// expected number of module ID's
	unsigned int expect = packet.Size();

	// get list of sibling module ID's
	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + expect);

	// copy array of module ID's since we reuse the response packet buffer
	size_t count = expect / 2;
	const uint16_t *begin = (const uint16_t*) (response.GetData() + SB_JLPACKET_HEADER_SIZE);
	const uint16_t *end = begin + count;
	vector<uint16_t> ids(begin, end);

	CodFileBuilder builder(cod_name, count);

	// save each block of data
	for( size_t i = 0; i < count; i++ ) {
		SaveData(packet, be_btohs(ids[i]), builder, output);
	}

	builder.WriteFooter(output);
}

void JavaLoader::DeviceInfo(JLDeviceInfo &info)
{
	Data command(-1, 8), data(-1, 8), response;
	JLPacket packet(command, data, response);

	packet.DeviceInfo();

	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::DeviceInfo", packet.Command());
	}

	m_socket->Receive(response);

	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE + SB_JLDEVICEINFO_SIZE);
	MAKE_JLPACKET(rpack, response);

	info.HardwareId = be_btohl(rpack->u.devinfo.hardware_id);
	info.Pin = be_btohl(rpack->u.devinfo.pin);
	info.OsVersion = be_btohl(rpack->u.devinfo.os_version);
	info.VmVersion = be_btohl(rpack->u.devinfo.vm_version);
	info.RadioId = be_btohl(rpack->u.devinfo.radio_id);
	info.VendorId = be_btohl(rpack->u.devinfo.vendor_id);
	info.ActiveWafs = be_btohl(rpack->u.devinfo.active_wafs);

	packet.OsMetrics();

	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::DeviceInfo", packet.Command());
	}

	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE);

	size_t offset = SB_JLPACKET_HEADER_SIZE;
	size_t size = response.GetSize()-offset;
	unsigned char* buf = info.OsMetrics.GetBuffer(size);
	memcpy(buf, response.GetData()+offset, size);
	info.OsMetrics.ReleaseBuffer(size);

	packet.BootromMetrics();

	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::DeviceInfo", packet.Command());
	}

	m_socket->Receive(response);
	Protocol::CheckSize(response, SB_JLPACKET_HEADER_SIZE);

	offset = SB_JLPACKET_HEADER_SIZE;
	size = response.GetSize()-offset;
	buf = info.BootromMetrics.GetBuffer(size);
	memcpy(buf, response.GetData()+offset, size);
	info.BootromMetrics.ReleaseBuffer(size);
}

void JavaLoader::Wipe(bool apps, bool fs)
{
	Data command(-1, 8), data(-1, 8), response;
	JLPacket packet(command, data, response);

	if( apps ) {
		packet.WipeApps();
		m_socket->Packet(packet);

		if( packet.Command() != SB_COMMAND_JL_ACK ) {
			ThrowJLError("JavaLoader::WipeApps", packet.Command());
		}
	}

	if( fs ) {
		packet.WipeFs();
		m_socket->Packet(packet);

		if( packet.Command() != SB_COMMAND_JL_ACK ) {
			ThrowJLError("JavaLoader::WipeFs", packet.Command());
		}
	}
}

void JavaLoader::LogStackTraces()
{
	Data command(-1, 8), data(-1, 8), response;
	JLPacket packet(command, data, response);

	packet.LogStackTraces();
	m_socket->Packet(packet);

	if( packet.Command() != SB_COMMAND_JL_ACK ) {
		ThrowJLError("JavaLoader::LogStackTraces", packet.Command());
	}
}

}} // namespace Barry::Mode

