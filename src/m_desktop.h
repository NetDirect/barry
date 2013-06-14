///
/// \file	m_desktop.h
///		Mode class for the Desktop mode
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "dll.h"
#include "m_mode_base.h"
#include "data.h"
#include "socket.h"
#include "record.h"
#include "parser.h"
#include "builder.h"

namespace Barry {

// forward declarations
class Parser;
class IConverter;

namespace Mode {

class DBLoader;

//
// Desktop class
//
/// The main interface class to the device databases.
///
/// To use this class, use the following steps:
///
///	- Create a Controller object (see Controller class for more details)
///	- Create this Mode::Desktop object, passing in the Controller
///		object during construction
///	- Call Open() to open database socket and finish constructing.
///	- Call GetDBDB() to get the device's database database
///	- Call GetDBID() to get a database ID by name
///	- Call LoadDatabase() to retrieve and store a database
///
class BXEXPORT Desktop : public Mode
{
	friend class DBLoader;

public:
	enum CommandType { Unknown, DatabaseAccess };

private:
	// packet data
	Data m_command, m_response;

	CommandTable m_commandTable;
	DatabaseDatabase m_dbdb;

	// external objects (optional, can be null)
	const IConverter *m_ic;

protected:
	void LoadCommandTable();
	void LoadDBDB();

	//////////////////////////////////
	// overrides

	virtual void OnOpen();

public:
	Desktop(Controller &con);
	Desktop(Controller &con, const IConverter &ic);
	~Desktop();

	//////////////////////////////////
	// meta access

	/// Returns DatabaseDatabase object for this connection.
	/// Must call Open() first, which loads the DBDB.
	const DatabaseDatabase& GetDBDB() const { return m_dbdb; }
	unsigned int GetDBID(const std::string &name) const;
	unsigned int GetDBCommand(CommandType ct);

	void SetIConverter(const IConverter &ic);

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
	void ClearDatabase(unsigned int dbId);
	void SaveDatabase(unsigned int dbId, Builder &builder);

	template <class RecordT, class StorageT> void LoadDatabaseByType(StorageT &store);
	template <class RecordT, class StorageT> void SaveDatabaseByType(StorageT &store);

	template <class StorageT> void LoadDatabaseByName(const std::string &name, StorageT &store);
	template <class StorageT> void SaveDatabaseByName(const std::string &name, StorageT &store);

	template <class RecordT> void AddRecordByType(uint32_t recordId, const RecordT &rec);

};

// used to hold internal-only state
struct DBLoaderData;

//
// DBLoader
//
/// Database Loader operation class.  Encapsulates the load / save
/// logic of Desktop::LoadDatabase() and someday Desktop::SaveDatabase()
/// in such a way that the loading of individual records is
/// controllable by the user, instead of using the parser callback mechanism.
///
/// This class can be reused to load / save multiple databases, but
/// do not call Desktop members while a load operation is in progress.
///
class BXEXPORT DBLoader
{
	Desktop &m_desktop;
	Data m_send;
	bool m_loading;
	std::string m_dbName;
	DBLoaderData *m_loader;

public:
	explicit DBLoader(Desktop &desktop);
	~DBLoader();

	/// Do not call Desktop members if this is true.
	bool IsBusy() const { return m_loading; }

	// caller-controllable load/save operations... if
	// these functions return true, then new data has
	// just been loaded into the data object passed to
	// the constructor
	//
	// Both of these functions use a DBData object in order
	// to pass buffers from application code all the way down
	// to the socket level, to avoid copies wherever possible.
	bool StartDBLoad(unsigned int dbId, DBData &data);
	bool GetNextRecord(DBData &data);
};

} // namespace Barry::Mode





//
// DeviceBuilder
//
/// Takes a list of database dbId's and behaves like a Builder,
/// trying to avoid copies where possible on the device loading end.
///
class BXEXPORT DeviceBuilder : public Builder
{
	typedef unsigned int				dbid_type;

	struct DBLabel
	{
		dbid_type id;
		std::string name;

		DBLabel(dbid_type id, const std::string &name)
			: id(id)
			, name(name)
		{
		}
	};

	typedef std::vector<DBLabel>			list_type;

	// list of databases to fetch during build
	list_type m_dbIds;
	list_type::iterator m_current;
	bool m_started;

	Mode::Desktop &m_desktop;

	// loader object to use optimized batch loading while
	// giving per-record control
	Mode::DBLoader m_loader;

public:
	explicit DeviceBuilder(Mode::Desktop &desktop);

	// searches the dbdb from the desktop to find the dbId,
	// returns false if not found, and adds it to the list of
	// databases to retrieve if found
	bool Add(const std::string &dbname);

	// adds all databases found in the given dbdb
	void Add(const Barry::DatabaseDatabase &dbdb);

	/// sets the internal iterator to the start of the list
	/// in order to perform a fresh run
	void Restart() { m_current = m_dbIds.begin(); m_started = false; }

	//
	// Builder overrides
	//

	// has both BuildRecord() and Retrieve() functionality,
	// and uses data all the way down to the socket level copy
	virtual bool BuildRecord(DBData &data, size_t &offset,
		const IConverter *ic);
	virtual bool FetchRecord(DBData &data, const IConverter *ic);
	virtual bool EndOfFile() const;
};


//
// DeviceParser
//
/// A parser class that "parses" raw data into a device.  Basically this
/// is a pipe-oriented way to call SaveDatabase().
///
/// Note that this is a multi-record parser.  For each incoming DBData
/// that has a new DBName, a new save will be started.  There is no
/// way to filter out records, except via the callback, so the easiest
/// way to filter out records by database name is on the Builder side.
///
class BXEXPORT DeviceParser : public Barry::Parser
{
public:
	enum WriteMode {
		/// Similar to SaveDatabase().  Erases all records from
		/// the existing database and then uploads all new records.
		ERASE_ALL_WRITE_ALL,

		/// Adds any new records, and for records with Unique IDs
		/// that already exist, overwrite them.
		INDIVIDUAL_OVERWRITE,

		/// Adds any new records, but if a record exists with the
		/// current Unique ID, skip that record and don't write it
		/// to the device.
		ADD_BUT_NO_OVERWRITE,

		/// Adds all incoming records as brand new records, generating
		/// a new Unique ID for each one, and leaving any existing
		/// records intact.
		ADD_WITH_NEW_ID,

		/// Calls the virtual function DecideWrite(...) for each
		/// record, passing in the data.  DecideWrite() returns one
		/// of these WriteMode values.
		DECIDE_BY_CALLBACK,

		/// Primarily used by DecideWrite(), and causes the current
		/// record to not be written.
		DROP_RECORD
	};

private:
	Mode::Desktop &m_desktop;
	WriteMode m_mode;

	std::string m_current_db;
	unsigned int m_current_dbid;
	RecordStateTable m_rstate;

protected:
	void StartDB(const DBData &data, const IConverter *ic);
	void WriteNext(const DBData &data, const IConverter *ic);

public:
	DeviceParser(Mode::Desktop &desktop, WriteMode mode);
	virtual ~DeviceParser();

	/// Callback... you must derive and override this if you use
	/// the DECIDE_BY_CALLBACK mode.
	/// May be called multiple times per record.
	virtual WriteMode DecideWrite(const DBData &record) const
	{
		return DROP_RECORD;
	}

	/// Parser overrides
	virtual void ParseRecord(const DBData &data, const IConverter *ic);
};


} // namespace Barry

#endif

