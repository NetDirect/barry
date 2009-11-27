///
/// \file	configfile.h
///		Barry configuraion class, for one device PIN
///

/*
    Copyright (C) 2007-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_CONFIGFILE_H__
#define __BARRY_CONFIGFILE_H__

#include "dll.h"
#include "record.h"
#include "pin.h"
#include <string>

namespace Barry {

class BXEXPORT ConfigFile
{
public:
	class DBListType : public std::vector<std::string>
	{
	public:
		bool IsSelected(const std::string &dbname) const;
	};

private:
	// meta data
	Barry::Pin m_pin;
	std::string m_path;		// /path/to/config/dir  without trailing slash
	std::string m_filename;		// /path/to/config/dir/filename
	bool m_loaded;
	std::string m_last_error;

	// configuration data
	DBListType m_backupList;
	DBListType m_restoreList;
	std::string m_deviceName;
	bool m_promptBackupLabel;	// if true, prompt the user on every
					// backup for a string to label the
					// backup file with
protected:
	void BuildFilename();
	void BuildDefaultPath();
	void Clear();
	void Load();
	bool CheckPath();

public:
	/// Loads config file for the given pin, and ends up in an
	/// unenlightened state.  Throws ConfigFileError on error,
	/// but it is not an error if the config does not exist.
	/// Never use this if you have a DatabaseDatabase object!
	/// This ctor is only for temporary loading of config data.
	explicit ConfigFile(Barry::Pin pin);

	/// Opens and loads config file for given pin, and calls Enlighten
	/// Throws ConfigFileError on error.  Should never fail unless
	/// passed a bad pin, or if unable to get current user info.
	ConfigFile(Barry::Pin pin, const Barry::DatabaseDatabase &db);

	~ConfigFile();

	//
	// data access
	//

	const std::string& get_last_error() const { return m_last_error; }

	const std::string& GetPath() const { return m_path; }
	const std::string& GetFilename() const { return m_filename; }
	const DBListType& GetBackupList() const { return m_backupList; }
	const DBListType& GetRestoreList() const { return m_restoreList; }
	const std::string& GetDeviceName() const { return m_deviceName; }
	bool HasDeviceName() const { return m_deviceName.size(); }
	bool PromptBackupLabel() const { return m_promptBackupLabel; }

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

	void SetDeviceName(const std::string &name);
	void SetBackupPath(const std::string &path);
	void SetPromptBackupLabel(bool prompt = true);

	//
	// Utility functions
	//

	/// Checks that the path in path exists, and if not, creates it.
	/// Returns false if unable to create path, true if ok.
	static bool CheckPath(const std::string &path, std::string *perr = 0);
};

} // namespace Barry

#endif

