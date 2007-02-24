///
/// \file	controller.h
///		High level BlackBerry API class
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

#ifndef __BARRY_CONTROLLER_H__
#define __BARRY_CONTROLLER_H__

#include "usbwrap.h"
#include "probe.h"
#include "socket.h"
#include "record.h"

/// Project namespace, containing all related functions and classes.
/// This is the only namespace applications should be concerned with,
/// for now.
namespace Barry {

// forward declarations
class Parser;
class Builder;
class DBPacket;

//
// Controller class
//
/// The main interface class.  This class coordinates the communication to
/// a single handheld.
///
/// To use this class, use the following steps:
///
///	- Probe the USB bus for matching devices with the Probe class
///	- Pass one of the probe results into the Controller constructor
///		to connect
///	- Call OpenMode() to select the desired mode.  This will fill all
///		internal data structures for that mode, such as the
///		Database Database in Desktop mode.
///		NOTE: only Desktop mode is currently implemented.
///	- Call GetDBDB() to get the device's database database
///	- Call GetDBID() to get a database ID by name
///	- In Desktop mode, call LoadDatabase() to retrieve and store a database
///
class Controller
{
	friend class Barry::DBPacket;

public:
	/// Handheld mode type
	enum ModeType {
		Unspecified,		//< default on start up
		Bypass,			//< unsupported, unknown
		Desktop,		//< desktop mode required for database
					//< operation
		JavaLoader		//< unsupported
	};
	enum CommandType { Unknown, DatabaseAccess };

private:
	Usb::Device m_dev;
	Usb::Interface *m_iface;
	uint32_t m_pin;

	Socket m_socket;

	CommandTable m_commandTable;
	DatabaseDatabase m_dbdb;

	ModeType m_mode;

protected:
	void SelectMode(ModeType mode, uint16_t &socket);
	unsigned int GetCommand(CommandType ct);

	void LoadCommandTable();
	void LoadDBDB();

public:
	Controller(const ProbeResult &device);
	~Controller();

	//////////////////////////////////
	// meta access

	/// Returns DatabaseDatabase object for this connection.
	/// Must call OpenMode() to select Desktop mode first
	const DatabaseDatabase& GetDBDB() const { return m_dbdb; }
	unsigned int GetDBID(const std::string &name) const;

	//////////////////////////////////
	// general operations
	void OpenMode(ModeType mode);

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

} // namespace Barry

#endif

