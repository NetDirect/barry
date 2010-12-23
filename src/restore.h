///
/// \file	restore.h
///		Builder class for restoring from Barry Backup files
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_RESTORE_H__
#define __BARRYBACKUP_RESTORE_H__

#include "dll.h"
#include "builder.h"
#include "configfile.h"
#include <string>
#include <vector>
#include <memory>

// forward declarations
namespace reuse {
	class TarFile;
}

namespace Barry {

//
// Restore
//
/// Barry Backup Restore builder class.  This class is suitable
/// to be used as a builder object anywhere a builder object is
/// accepted.  It reads from a Barry Backup tar.gz backup file,
/// and builds records in a staged manner.
///
/// If a backup file contains more than one database (for example
/// both Address Book and Calendar), then it will build one database
/// first, return false on Retrieve(), and then build the next.
/// If Retrieve() returns false, but EndOfFile() also returns false,
/// then more databases are available.
///
/// The idea is that you can call Desktop::SaveDatabase() multiple
/// times with this same Restore object, for all the databases in
/// the backup file.
///
/// It is safe to call Retrieve() multiple times, so when first
/// starting a restore:
///	- call the constructor
///	- call AddDB() with any filters
///	- then call Retrieve(), which will grab the first record,
///	  and make GetDBName() valid.
///
class BXEXPORT Restore : public Barry::Builder
{
public:
	typedef Barry::ConfigFile::DBListType		DBListType;

private:
	enum RetrievalState
	{
		RS_EMPTY,	// no record loaded
		RS_UNKNOWN,	// record data loaded, but not yet checked
				// whether this is part of current database
		RS_NEXT,	// record loaded, part of current database
		RS_DBEND,	// next record loaded, but end-of-database
				// not yet processed by Builder API
		RS_EOF		// no recrd loaded, end of tar file
	};

	DBListType m_dbList;

	std::string m_tarpath;
	std::auto_ptr<reuse::TarFile> m_tar;

	bool m_default_all_db;
	RetrievalState m_tar_record_state;
	uint8_t m_rec_type;
	uint32_t m_unique_id;
	std::string m_current_dbname;
	Barry::Data m_record_data;
	std::string m_tar_id_text;

protected:
	static bool SplitTarPath(const std::string &tarpath,
		std::string &dbname, std::string &dbid_text,
		uint8_t &dbrectype, uint32_t &dbid);

	bool IsSelected(const std::string &dbName) const;
	RetrievalState Retrieve(Data &record_data);


public:
	/// If default_all_db is true, and none of the Add*() functions
	/// are called (meaning that Restore has an empty database list),
	/// then all records are restored.  If false in this situation,
	/// nothing is restored.
	///
	/// If any of the Add*() functions are called, then the database
	/// list takes precedence, and default_all_db has no effect.
	///
	explicit Restore(const std::string &tarpath,
			bool default_all_db = true);
	~Restore();

	/// Add database name to the read filter.
	void AddDB(const std::string &dbName);

	/// Add all database names in dblist to the read filter
	/// This function is additive.
	void Add(const DBListType &dbList);

	// Skip the current DB, in case of error, or preference
	void SkipCurrentDB();

	/// Loads the given file, and counts all records according
	/// to the current read filter.  Does not use the main
	/// Restore file, but opens the file separately.
	/// It is safe to call this function as often as needed.
	unsigned int GetRecordTotal() const;

	/// Static version of above call
	static unsigned int GetRecordTotal(const std::string &tarpath,
		const DBListType &dbList, bool default_all_db);

	/// If this function returns true, it fills data with the
	/// meta data that the next call to BuildRecord() will retrieve.
	/// This is useful for applications that need to setup a manual
	/// call to Desktop::SaveDatabase().
	bool GetNextMeta(DBData &data);

	// Barry::Builder overrides
	bool BuildRecord(Barry::DBData &data, size_t &offset,
		const Barry::IConverter *ic);
	bool FetchRecord(Barry::DBData &data, const Barry::IConverter *ic);
	bool EndOfFile() const;
};

} // namespace Barry

#endif

