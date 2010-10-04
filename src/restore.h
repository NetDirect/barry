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
/// starting a restore, call the constructor, call AddDB() with
/// any filters, and then call Retrieve(), which will grab the
/// first record, and make GetDBName() valid.
///
class BXEXPORT Restore : public Barry::Builder
{
public:
	typedef Barry::ConfigFile::DBListType		DBListType;

private:
	DBListType m_dbList;

	std::auto_ptr<reuse::TarFile> m_tar;

	bool m_default_all_db;
	bool m_end_of_tar;
	bool m_tar_record_loaded;
	uint8_t m_rec_type;
	uint32_t m_unique_id;
	std::string m_current_dbname;
	std::string m_record_data;
	std::string m_tar_id_text;

protected:
	static bool SplitTarPath(const std::string &tarpath,
		std::string &dbname, std::string &dbid_text,
		uint8_t &dbrectype, uint32_t &dbid);

	bool IsSelected(const std::string &dbName) const;


public:
	explicit Restore(const std::string &tarpath,
			bool default_all_db = true);
	~Restore();

	/// Add database names to the read filter.
	///
	/// If default_all_db is true and you don't call this function,
	/// all databases are read.
	/// If default_all_db is false and you don't call this function,
	/// no databases are read.
	/// Regardless of default_all_db, if you call this function one
	/// or more times, only the specified databases are read.
	void AddDB(const std::string &dbName);

	// Skip the current DB, in case of error, or preference
	void SkipCurrentDB();

	/// Loads the given file, and counts all records according
	/// to the current read filter.  Does not use the main
	/// Restore file, but opens the file separately.
	/// It is safe to call this function as often as needed.
	unsigned int GetRecordTotal(const std::string &tarpath) const;

	// Barry::Builder overrides
	virtual bool Retrieve();
	virtual std::string GetDBName() const;  // only valid after a Retrieve()
	virtual uint8_t GetRecType() const;
	virtual uint32_t GetUniqueId() const;
	virtual bool EndOfFile() const { return m_end_of_tar; }
	virtual void BuildHeader(Barry::Data &data, size_t &offset);
	virtual void BuildFields(Barry::Data &data, size_t &offset, const Barry::IConverter *ic);
	virtual void BuildDone();
};

} // namespace Barry

#endif

