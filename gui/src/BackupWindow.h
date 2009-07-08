///
/// \file	BackupWindow.h
///		GUI window class
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

#include <gtkmm.h>
#include <libglademm.h>
#include <memory>
#include "DeviceIface.h"
#include "ConfigFile.h"

class BackupWindow : public Gtk::Window
{
	// columns in DeviceList
	class Columns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		Gtk::TreeModelColumn<Glib::ustring> m_pin_text;
		Gtk::TreeModelColumn<uint32_t> m_pin;

		Columns()
		{
			add(m_pin_text);
			add(m_pin);
		}
	};
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
	Gtk::MenuItem *m_pEditConfig;
	Gtk::ProgressBar *m_pProgressBar;
	Gtk::Statusbar *m_pStatusBar;
	Gtk::Entry *m_pDatabaseEntry;
	Gtk::Button *m_pBackupButton, *m_pRestoreButton;
	Gtk::Label *m_pDeviceLabel;
	Gtk::ComboBox *m_pDeviceList;
	
	// objects used by DeviceList
	Gtk::TreeView *m_pTree;
	Columns m_Columns;
	Glib::RefPtr<Gtk::ListStore> m_pListStore;

	// index of active device
	int m_active_device;
	
	// number of devices
	int m_device_count;

	// state
	bool m_scanned;
	bool m_connected;
	bool m_working;		// true if backup or restore in progress
	bool m_thread_error;

protected:
	void Scan();
	void Connect();
	void CheckDeviceName();
	void SetDeviceName(const std::string &name);
	void SetWorkingMode(const std::string &taskname);
	void ClearWorkingMode();
	void UpdateProgress();
	bool PromptForRestoreTarball(std::string &restoreFilename,
		const std::string &start_path);
	bool CheckWorkingDevice();
	void ResetDeviceList();
	void SetActiveDevice(unsigned int index, bool setList = true);

public:
	BackupWindow(BaseObjectType *cobject, const Glib::RefPtr<Gnome::Glade::Xml> &xml);
	~BackupWindow();

	// handler for exceptions that happen in signal calls
	void signal_exception_handler();

	// signal handlers
	void on_backup();
	void on_restore();
	void on_device_change();
	void on_file_quit();
	void on_edit_config();
	void on_help_about();
	bool on_startup();
	void on_thread_progress();
	void on_thread_error();
	void on_thread_done();
	void on_thread_erase_db();
};

// this class is used by functions to
// prevent abnormal returns from
// failing to update the status bar
class StatusBarHandler
{
	Gtk::Statusbar *psb;
	public:
	StatusBarHandler(Gtk::Statusbar *pStatusBar, const std::string Message) : psb(pStatusBar)
	{
		psb->push(Message);
		psb->show_now();
	}
	~StatusBarHandler()
	{
		psb->pop();
	}
};
