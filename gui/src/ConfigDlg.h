///
/// \file	ConfigDlg.h
///		Dialog wrapper class for user selection of device databases
///

/*
    Copyright (C) 2007-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYBACKUP_CONFIGDLG_H__
#define __BARRYBACKUP_CONFIGDLG_H__

#include <gtkmm.h>
#include <memory>
#include <barry/barry.h>

namespace Barry {
	class DatabaseDatabase;
}

class ConfigDlg
{
	// external data
	const Barry::DatabaseDatabase &m_dbdb;

	// Widgets
	std::auto_ptr<Gtk::Dialog> m_pDialog;
	Gtk::Entry *m_pDeviceNameEntry;
	Gtk::CheckButton *m_pPromptBackupLabelCheck;
	Gtk::FileChooserButton *m_pBackupPath;

	// config data
	Barry::ConfigFile::DBListType m_backupList;
	Barry::ConfigFile::DBListType m_restoreList;
	std::string m_deviceName;
	std::string m_backupPath;
	bool m_promptBackupLabel;

public:
	ConfigDlg(const Barry::DatabaseDatabase &dbdb,
		const Barry::ConfigFile &config);
	~ConfigDlg();

	const Barry::ConfigFile::DBListType& GetBackupList() const { return m_backupList; }
	const Barry::ConfigFile::DBListType& GetRestoreList() const { return m_restoreList; }
	const std::string& GetDeviceName() const { return m_deviceName; }
	const std::string& GetBackupPath() const { return m_backupPath; }
	bool GetPromptBackupLabel() const { return m_promptBackupLabel; }

	int run();

	// signals
	void on_configure_backup();
	void on_configure_restore();
};

#endif

