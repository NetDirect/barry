///
/// \file	m_desktop.h
///		Mode class for the Desktop mode
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_M_DESKTOP_H__
#define __BARRY_M_DESKTOP_H__

//#include "controller.h"
//#include "usbwrap.h"
//#include "probe.h"
#include "socket.h"
#include "record.h"
//#include "data.h"

/// Project namespace, containing all related functions and classes.
/// This is the only namespace applications should be concerned with,
/// for now.
namespace Barry {

// forward declarations
class Parser;
class Builder;
class DBPacket;
class Controller;

namespace Mode {

//
// Desktop class
//
/// The main interface class to the device databases.
///
/// To use this class, use the following steps:
///
///	- Probe the USB bus for matching devices with the Probe class
///	- Pass one of the probe results into the Controller constructor
///		to connect
///	- Pass the Controller object into the Desktop ctor
///	- Call Open() to open database socket 
select the desired mode.  This will fill all
///		internal data structures for that mode, such as the
///		Database Database in Desktop mode.
///		NOTE: only Desktop mode is currently implemented.
///	- Call GetDBDB() to get the device's database database
///	- Call GetDBID() to get a database ID by name
///	- In Desktop mode, call LoadDatabase() to retrieve and store a database
///
class Desktop
{
	friend class Barry::DBPacket;

public:
	enum CommandType { Unknown, DatabaseAccess };

private:
	Controller &m_con;

	SocketHandle m_socket;

	CommandTable m_commandTable;
	DatabaseDatabase m_dbdb;

	uint16_t m_ModeSocket;			// socket recommended by device
						// when mode was selected

	// tracking of open Desktop socket, and the need to reset
//	bool m_halfOpen;

protected:
	unsigned int GetCommand(CommandType ct);

	void LoadCommandTable();
	void LoadDBDB();

public:
	Desktop(Controller &con);
	~Desktop();

	//////////////////////////////////
	// meta access

	/// Returns DatabaseDatabase object for this connection.
	/// Must call Open() first, which loads the DBDB.
	const DatabaseDatabase& GetDBDB() const { return m_dbdb; }
	unsigned int GetDBID(const std::string &name) const;

	//////////////////////////////////
	// general operations
	void Open(const char *password = 0);
	void RetryPassword(const char *password);

	//////////////////////////////////
	// Desktop mode - database specific

	// dirty flag related functions, for sync operations
	void GetRecordStateTable(unsigned int dbId, RecordStateTable &result);
	void AddRecord(unsigned int dbId, Builder &build); // RecordId is
		// retrieved from build, and duplicate IDs are allowed,
		// but *not* recommended!
	void GetRecord(unsigned int dbId, unsigned int stateTableIndex, Parser &parser);
	void SetRecord(unsigned int dbId, unsigned int stateTableIndex, Builder &build);
	void ClearDirty(unsigned int dbId, unsigned int stateTableIndex);
	void DeleteRecord(unsigned int dbId, unsigned int stateTableIndex);

	// pure load/save operations
	void LoadDatabase(unsigned int dbId, Parser &parser);
	void SaveDatabase(unsigned int dbId, Builder &builder);

	template <class RecordT, class StorageT> void LoadDatabaseByType(StorageT &store);
	template <class RecordT, class StorageT> void SaveDatabaseByType(StorageT &store);

	template <class StorageT> void LoadDatabaseByName(const std::string &name, StorageT &store);
	template <class StorageT> void SaveDatabaseByName(const std::string &name, StorageT &store);

	template <class RecordT> void AddRecordByType(uint32_t recordId, const RecordT &rec);

};

}} // namespace Barry::Mode

#endif

