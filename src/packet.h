///
/// \file	packet.h
///		Low level protocol packet builder class.
///		Has knowledge of specific protocol commands in order
///		to hide protocol details behind an API.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_PACKET_H__
#define __BARRY_PACKET_H__

#include <string>
#include <stdint.h>
#include "protocol.h"

namespace Barry { class Data; }

namespace Barry {

// forward declarations
class Parser;
class Builder;
class SocketZero;
class Socket;
class IConverter;
namespace Mode {
	class Desktop;
	class JavaLoader;
}

class Packet
{
	friend class SocketZero;
	friend class Socket;

protected:
	Data &m_send, &m_receive;

	Data& GetSend() { return m_send; }
	Data& GetReceive() { return m_receive; }

public:
	Packet(Data &send, Data &receive)
		: m_send(send), m_receive(receive)
		{}
	virtual ~Packet() {}

	//////////////////////////////////
	// common response analysis

	unsigned int Command() const;	// throws Error if receive isn't big enough
};

//
// ZeroPacket class
//
/// Provides an API for building and analyzing socket-0 protocol packets.
/// This class relies on 2 external objects: a send and receive Data buffer.
///
/// Note that the receive buffer may be modified
/// during a packet send, and this class provides API helpers
/// to analyze the results.
///
class ZeroPacket : public Packet
{
	friend class Socket;

public:
	ZeroPacket(Data &send, Data &receive);
	~ZeroPacket();

	//////////////////////////////////
	// meta access

	//////////////////////////////////
	// packet building

	void GetAttribute(unsigned int object, unsigned int attribute);
	void Echo(uint64_t us_ticks);
	void Reset();


	//////////////////////////////////
	// response analysis

	unsigned int ObjectID() const;
	unsigned int AttributeID() const;
	uint32_t ChallengeSeed() const;
	unsigned int RemainingTries() const;
	unsigned int SocketResponse() const;
	unsigned char SocketSequence() const;
	uint8_t CommandResponse() const;
};


//
// DBPacket class
//
/// Provides an API for building and analyzing raw DB protocol packets.
/// This class relies on 3 external objects: a Mode::Desktop object,
/// a send Data buffer, and a receive data buffer.  Socket and
/// connection details are retrieved on a readonly basis from the
/// Mode::Desktop object, but both send and receive buffers can be
/// modified.
///
/// Note that the receive buffer may be modified
/// during a packet send, and this DBPacket class provides API helpers
/// to analyze the results.
///
class DBPacket : public Packet
{
	friend class Socket;

private:
	Mode::Desktop &m_con;
	unsigned int m_last_dbop;	// last database operation

protected:

public:
	DBPacket(Mode::Desktop &con, Data &send, Data &receive);
	~DBPacket();

	//////////////////////////////////
	// meta access

	//////////////////////////////////
	// packet building

	// commands that correspond to the DB operation
	// constants in protocol.h
	void ClearDatabase(unsigned int dbId);
	void GetDBDB();
	void GetRecordStateTable(unsigned int dbId);
	void SetRecordFlags(unsigned int dbId, unsigned int stateTableIndex, uint8_t flag1);
	void DeleteRecordByIndex(unsigned int dbId, unsigned int stateTableIndex);
	void GetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex);
	bool SetRecordByIndex(unsigned int dbId, unsigned int stateTableIndex, Builder &build, const IConverter *ic);
	void GetRecords(unsigned int dbId);
	bool SetRecord(unsigned int dbId, Builder &build, const IConverter *ic);


	//////////////////////////////////
	// response analysis

	// DB command response functions
	unsigned int ReturnCode() const;	// throws FIXME if packet doesn't support it
	unsigned int DBOperation() const; // throws Error on size trouble

	bool Parse(Parser &parser, const IConverter *ic); // switches based on last m_send command

	// response parsers
};


//
// JLPacket class
//
/// Provides an API for building and analyzing raw Javaloader protocol packets.
/// This class relies on 3 external objects:
/// a command send Data buffer (which can be fairly small), a data
/// or argument send Data buffer, and a receive data buffer.  Socket and
/// connection details are retrieved on a readonly basis from the
/// Mode::JavaLoader object, but all buffers can be modified.
///
/// Note that the receive buffer may be modified
/// during a packet send, and this JLPacket class provides API helpers
/// to analyze the results.
///
class JLPacket : public Packet
{
	friend class Socket;

private:
	Data &m_cmd, &m_data;
	int m_last_set_size;

public:
	JLPacket(Data &cmd, Data &send, Data &receive);
	~JLPacket();

	//////////////////////////////////
	// meta access

	bool HasData() const	{ return m_last_set_size == 2; }
	Data& GetReceive()	{ return m_receive; }

	//////////////////////////////////
	// packet building

	// commands that correspond to the operation
	// constants in protocol.h

	// returns 1 or 2 depending on whether cmd or cmd+send are available
	int SimpleCmd(uint8_t cmd, uint8_t unknown = 0, uint16_t size = 0);
	int SimpleData(const void *data, uint16_t size);
	int BigEndianData(uint16_t value);
	int BigEndianData(uint32_t value);

	int Hello()		{ return SimpleCmd(SB_COMMAND_JL_HELLO); }
	int Goodbye()		{ return SimpleCmd(SB_COMMAND_JL_GOODBYE); }
	int SetUnknown1();
	int SetCodFilename(const std::string &filename);
	int SetCodSize(off_t size);
	int SetTime(time_t when);
	int GetScreenshot();
	int GetData()		{ return SimpleCmd(SB_COMMAND_JL_SEND_DATA); }
	int DeviceInfo()	{ return SimpleCmd(SB_COMMAND_JL_DEVICE_INFO); }
	int OsMetrics()		{ return SimpleCmd(SB_COMMAND_JL_OS_METRICS); }
	int BootromMetrics()	{ return SimpleCmd(SB_COMMAND_JL_BOOTROM_METRICS); }
	int GetDirectory()	{ return SimpleCmd(SB_COMMAND_JL_GET_DIRECTORY); }
	int GetSubDir(uint16_t id);
	int GetDirEntry(uint8_t entry_cmd, uint16_t id);
	int Erase(uint16_t cmd, uint16_t id);
	int GetEventlog()	{ return SimpleCmd(SB_COMMAND_JL_GET_LOG); }
	int GetEventlogEntry(uint16_t entry_num);
	int ClearEventlog()	{ return SimpleCmd(SB_COMMAND_JL_CLEAR_LOG); }
	int SaveModule(uint16_t id);
	int PutData(const void *data, uint16_t size);
	int WipeApps()		{ return SimpleCmd(SB_COMMAND_JL_WIPE_APPS); }
	int WipeFs()		{ return SimpleCmd(SB_COMMAND_JL_WIPE_FS); }
	int LogStackTraces()	{ return SimpleCmd(SB_COMMAND_JL_LOG_STRACES); }
	int ResetToFactory()	{ return SimpleCmd(SB_COMMAND_JL_RESET_FACTORY); }

	//////////////////////////////////
	// response analysis
	unsigned int Size();
};


//
// JDPacket class
//
/// Provides an API for building and analyzing raw JavaDebug protocol packets.
/// This class relies on 3 external objects:
/// a command send Data buffer (which can be fairly small), a data
/// or argument send Data buffer, and a receive data buffer.  Socket and
/// connection details are retrieved on a readonly basis from the
/// Mode::JavaDebug object, but all buffers can be modified.
///
/// Note that the receive buffer may be modified
/// during a packet send, and this JDPacket class provides API helpers
/// to analyze the results.
///
class JDPacket : public Packet
{
	friend class Socket;

private:
	Data &m_cmd;

public:
	JDPacket(Data &cmd, Data &receive);
	~JDPacket();

	//////////////////////////////////
	// meta access

	Data& GetReceive()	{ return m_receive; }

	//////////////////////////////////
	// packet building

	// commands that correspond to the operation
	// constants in protocol.h

	// returns 1 or 2 depending on whether cmd or cmd+send are available
	void SimpleCmd(uint8_t cmd);
	void ComplexCmd(uint8_t cmd, const void *param, uint16_t size = 0);

	void Unknown01();	// Command 0x53
	void Unknown02();	// Command 0x01
	void Unknown03();	// Command 0x6f
	void Unknown04();	// Command 0x8a
	void Unknown05();	// Command 0x90
	void Unknown06();	// Command 0x44
	void Unknown07();	// Command 0x45
	void Unknown08();	// Command 0x54
	void Unknown09();	// Command 0x33
	void Unknown10();	// Command 0x46
	void GetModulesList(uint32_t id);	// Command 0x8d
	void GetConsoleMessage();
	void Go();	// Command 0x02
	void GetStatus();	// Command 0x06

	//////////////////////////////////
	// response analysis
	unsigned int Size();
};


} // namespace Barry

#endif

