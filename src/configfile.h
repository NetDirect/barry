///
/// \file	configfile.h
///		Barry configuraion class, for one device PIN
///

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <iosfwd>

namespace Barry {

/// Creates a tar.gz filename using PIN + date + time + label.
/// Does not include any path, just returns a new filename.
BXEXPORT std::string MakeBackupFilename(const Barry::Pin &pin,
					const std::string &label);

class BXEXPORT ConfigFile
{
public:
	class BXEXPORT DBListType : public std::vector<std::string>
	{
	public:
		bool IsSelected(const std::string &dbname) const;

		DBListType& operator=(const DatabaseDatabase &dbdb);

		DBListType() {}
		DBListType(const DatabaseDatabase &dbdb)
		{
			operator=(dbdb);
		}
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
	bool m_autoSelectAll;		// if true, automatically select all
					// databases on every backup

protected:
	void BuildFilename();
	void BuildDefaultPath();
	void Clear();
	void Load();

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
	bool IsConfigLoaded() const { return m_loaded; }

	const std::string& GetPath() const { return m_path; }
//	const std::string& GetDeviceConfigFilename() const { return m_device_filename; }

	//
	// per-Device Configuration
	//
	const DBListType& GetBackupList() const { return m_backupList; }
	const DBListType& GetRestoreList() const { return m_restoreList; }
	const std::string& GetDeviceName() const { return m_deviceName; }
	bool HasDeviceName() const { return m_deviceName.size(); }
	bool PromptBackupLabel() const { return m_promptBackupLabel; }
	bool AutoSelectAll() const { return m_autoSelectAll; }

	//
	// operations
	//

	/// Saves current device's config, overwriting or creating a
	/// config file
	bool Save();

	/// Compares a given databasedatabase from a real device with the
	/// current config.  If not yet configured, initialize with valid
	/// defaults.
	void Enlighten(const Barry::DatabaseDatabase &db);

	//
	// per-Device Configuration setting
	//

	/// Sets list with new config
	void SetBackupList(const DBListType &list);
	void SetRestoreList(const DBListType &list);

	void SetDeviceName(const std::string &name);
	void SetBackupPath(const std::string &path);
	void SetPromptBackupLabel(bool prompt = true);
	void SetAutoSelectAll(bool asa = true);

	//
	// Utility functions
	//

	/// Checks that the path in path exists, and if not, creates it.
	/// Returns false if unable to create path, true if ok.
	static bool CheckPath(const std::string &path, std::string *perr = 0);
};

class BXEXPORT GlobalConfigFile
{
private:
	typedef std::map<std::string, std::string>		keymap_type;

	// meta data
	std::string m_path;	// /path/to/.barry   without trailing slash
	std::string m_filename;	// /path/to/.barry/config
	bool m_loaded;
	std::string m_last_error;
	std::string m_appname;

	// global configuration data
	Barry::Pin m_lastDevice;	// the last selected device in a GUI
	bool m_verboseLogging;

	// set/get key data
	keymap_type m_keymap;

protected:
	void BuildFilename();
	void Clear();
	void Load();

public:
	/// Loads the global config file.
	/// Throws ConfigFileError on error, but it is not an error
	/// if the config does not exist.
	GlobalConfigFile();

	/// Loads the global config file, as well as any application-specific
	/// keywords and variables.  Use this if you wish to make use of
	/// SetKey() and GetKey(), otherwise these functions will throw
	/// and exception since they don't have an application name.
	GlobalConfigFile(const std::string &appname);

	~GlobalConfigFile();

	//
	// data access
	//

	const std::string& get_last_error() const { return m_last_error; }
	bool IsConfigLoaded() const { return m_loaded; }
	const std::string& GetPath() const { return m_path; }

	//
	// Global Configuration data access
	//
	Barry::Pin GetLastDevice() const { return m_lastDevice; }
	bool VerboseLogging() const { return m_verboseLogging; }

	//
	// operations
	//

	/// Save the current global config, overwriting or creating as needed
	bool Save();

	/// Throws std::logic_error if not constructed with an appname
	void SetKey(const std::string &key, const std::string &value);

	/// Returns value for key, and returns default_value if key does not
	/// exist in the config file.
	/// Throws std::logic_error if not constructed with an appname
	std::string GetKey(const std::string &key,
		const std::string &default_value = "") const;

	//
	// Global Configuration setting
	//
	void SetLastDevice(const Barry::Pin &pin);
	void SetVerboseLogging(bool verbose = true);
};

BXEXPORT std::ostream& operator<< (std::ostream &os, const ConfigFile::DBListType &list);

} // namespace Barry

#endif

