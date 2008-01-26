///
/// \file	BackupWindow.h
///		GUI window class
///

/*
    Copyright (C) 2007-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <gtkmm.h>
#include <libglademm.h>
#include <memory>
#include "DeviceIface.h"
#include "ConfigFile.h"

class BackupWindow : public Gtk::Window
{
	// external data
	const Glib::RefPtr<Gnome::Glade::Xml> &m_xml;

	// Interface to Blackberry
	DeviceInterface m_dev;

	// signal exception handling connection
	sigc::connection m_signal_handler_connection;

	// data
	std::auto_ptr<ConfigFile> m_pConfig;
	Glib::Dispatcher m_signal_progress;
	Glib::Dispatcher m_signal_error;
	Glib::Dispatcher m_signal_done;
	Glib::Dispatcher m_signal_erase_db;
	int m_recordTotal;
	int m_finishedRecords;
	std::string m_modeName;

	// Widget objects
	Gtk::ProgressBar *m_pProgressBar;
	Gtk::Statusbar *m_pStatusBar;
	Gtk::Entry *m_pPINEntry, *m_pDatabaseEntry;
	Gtk::Button *m_pBackupButton, *m_pRestoreButton;

	// state
	bool m_scanned;
	bool m_working;		// true if backup or restore in progress
	bool m_thread_error;

protected:
	void ScanAndConnect();
	void SetWorkingMode(const std::string &taskname);
	void ClearWorkingMode();
	void UpdateProgress();
	bool PromptForRestoreTarball(std::string &restoreFilename,
		const std::string &start_path);

public:
	BackupWindow(BaseObjectType *cobject, const Glib::RefPtr<Gnome::Glade::Xml> &xml);
	~BackupWindow();

	// handler for exceptions that happen in signal calls
	void signal_exception_handler();

	// signal handlers
	void on_backup();
	void on_restore();
	void on_file_quit();
	void on_edit_config();
	void on_help_about();
	bool on_startup();
	void on_thread_progress();
	void on_thread_error();
	void on_thread_done();
	void on_thread_erase_db();
};

