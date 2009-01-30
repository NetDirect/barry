///
/// \file	m_javaloader.h
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

#ifndef __BARRY_M_JAVALOADER_H__
#define __BARRY_M_JAVALOADER_H__

#include "dll.h"
#include "socket.h"
#include "record.h"

namespace Barry {

// forward declarations
class Parser;
class Builder;
class Controller;
class CodFile;

class JLDirectoryEntry;

class BXEXPORT JLDirectory : public std::vector<JLDirectoryEntry>
{
public:
	typedef std::vector<JLDirectoryEntry>	BaseType;
	typedef BaseType::iterator		BaseIterator;
	typedef std::vector<uint16_t>		TableType;
	typedef TableType::iterator		TableIterator;

private:
	TableType m_idTable;

	int m_level;

public:
	JLDirectory(int level = 0);
	~JLDirectory();

	int Level() const { return m_level; }
	TableIterator TableBegin() { return m_idTable.begin(); }
	TableIterator TableEnd()   { return m_idTable.end(); }

	void ParseTable(const Data &table_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLDirectory &d) {
	d.Dump(os);
	return os;
}

class BXEXPORT JLDirectoryEntry
{
private:
	int m_level;

public:
	uint16_t Id;
	std::string Name;
	std::string Version;
	uint32_t CodSize;
	time_t Timestamp;

	JLDirectory SubDir;

public:
	explicit JLDirectoryEntry(int level);

	void Parse(uint16_t id, const Data &entry_packet);

	void Dump(std::ostream &os) const;
};
BXEXPORT inline std::ostream& operator<<(std::ostream &os, const JLDirectoryEntry &e) {
	e.Dump(os);
	return os;
}


class BXEXPORT JLScreenInfo {
public:
	uint16_t width;
	uint16_t height;

public:
	JLScreenInfo();
	~JLScreenInfo();
};


namespace Mode {

//
// JavaLoader class
//
/// The main interface class to the java program loader protocol
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::JavaLoader object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call LoadDatabase() to retrieve and store a database
///
class BXEXPORT JavaLoader
{
private:
	Controller &m_con;

	SocketHandle m_socket;

	uint16_t m_ModeSocket;			// socket recommended by device
						// when mode was selected

	bool m_StreamStarted;

protected:
	void GetDirectoryEntries(JLPacket &packet, uint8_t entry_cmd,
		JLDirectory &dir, bool include_subdirs);
	void GetDir(JLPacket &packet, uint8_t entry_cmd, JLDirectory &dir,
		bool include_subdirs);
	void ThrowJLError(const std::string &msg, uint8_t cmd);
	void DoErase(uint8_t cmd, const std::string &cod_name);

public:
	JavaLoader(Controller &con);
	~JavaLoader();

	//////////////////////////////////
	// primary operations - required before anything else

	void Open(const char *password = 0);
	void RetryPassword(const char *password);

	//////////////////////////////////
	// API
	void StartStream();
	void StopStream(void);

	// mid-stream operations
	void SendStream(const unsigned char *buffer, int size);
	void LoadApp(Barry::CodFile &cod);
	void SetTime(time_t when);
	void GetDirectory(JLDirectory &dir, bool include_subdirs);
	void GetScreenshot(JLScreenInfo &info, Data &image);
	void Erase(const std::string &cod_name);
	void ForceErase(const std::string &cod_name);
};

}} // namespace Barry::Mode

#endif

