///
/// \file	ConfigFile.h
///		BarryBackup GUI configuraion class
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_CONFIGFILE_H__
#define __BARRYBACKUP_CONFIGFILE_H__

#include <string>
#include <barry/record.h>
#include <stdexcept>

class ConfigFile
{
public:
	typedef std::vector<std::string>		DBListType;

	class ConfigFileError : public std::runtime_error
	{
	public:
		ConfigFileError(const char *msg) : std::runtime_error(msg) {}
		ConfigFileError(const char *msg, int err);
	};

private:
	// meta data
	std::string m_pin;
	std::string m_path;		// /path/to/config/dir  without trailing slash
	std::string m_filename;		// /path/to/config/dir/filename
	bool m_loaded;
	std::string m_last_error;

	// configuration data
	DBListType m_backupList;
	DBListType m_restoreList;

protected:
	void BuildFilename();
	void Clear();
	void Load();
	bool CheckPath();

public:
	/// Opens and loads config file for given pin, and calls Enlighten
	/// Throws ConfigFileError on error.  Should never fail unless
	/// passed a bad pin, or if unable to get current user info.
	explicit ConfigFile(const std::string &pin,
		const Barry::DatabaseDatabase &db);
	~ConfigFile();

	//
	// data access
	//

	const std::string& get_last_error() const { return m_last_error; }

	const std::string& GetPIN() const { return m_pin; }
	const std::string& GetPath() const { return m_path; }
	const std::string& GetFilename() const { return m_filename; }
	const DBListType& GetBackupList() const { return m_backupList; }
	const DBListType& GetRestoreList() const { return m_restoreList; }

	//
	// operations
	//

	/// Saves current config, overwriting or creating a config file
	bool Save();

	/// Compares a given databasedatabase from a real device with the
	/// current config.  If not yet configured, initialize with valid
	/// defaults.
	void Enlighten(const Barry::DatabaseDatabase &db);

	/// Sets list with new config
	void SetBackupList(const DBListType &list);
	void SetRestoreList(const DBListType &list);
};

#endif

