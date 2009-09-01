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
#include <vector>
#include "DeviceBus.h"
#include "ConfigFile.h"
#include "Thread.h"

class BackupWindow : public Gtk::Window
{
	// external data
	const Glib::RefPtr<Gnome::Glade::Xml> &m_xml;

	// dispatcher for updating thread info
	Glib::Dispatcher m_signal_update;

	// Bus of Blackberry devices
	DeviceBus m_bus;

	// vector for storing Threads
	std::vector<std::tr1::shared_ptr<Thread> > m_threads;

	// signal exception handling connection
	sigc::connection m_signal_handler_connection;

	// Widget objects
	Gtk::Statusbar *m_pStatusbar;
	Gtk::Button *m_pBackupButton, *m_pRestoreButton, 
		*m_pConfigButton, *m_pDisconnectButton, 
		*m_pDisconnectAllButton, *m_pReloadButton;
	Gtk::Label *m_pDeviceLabel;
	Gtk::TreeView *m_pDeviceList;

	// objects used by DeviceList
	class Columns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		Gtk::TreeModelColumn<unsigned int> m_id;
		Gtk::TreeModelColumn<Glib::ustring> m_pin;
		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_status;
		Gtk::TreeModelColumn<unsigned int> m_percentage;

		Columns()
		{
			add(m_id);
			add(m_pin);
			add(m_name);
			add(m_status);
			add(m_percentage);
		}
	};
	Columns m_columns;
	Glib::RefPtr<Gtk::ListStore> m_pListStore;
	Glib::RefPtr<Gtk::TreeSelection> m_pDeviceSelection;

	// number of devices
	unsigned int m_device_count;

	// pointer to active Thread;
	Thread *m_pActive;

	// whether scanned
	bool m_scanned;

	// statusbar message ID's
	guint m_last_status_id;

protected:
	void Scan();
	bool Connect(Thread *);
	void Disconnect(Thread *);
	void CheckDeviceName(Thread *);
	bool PromptForRestoreTarball(std::string &restoreFilename,
		const std::string &start_path);
	Thread *GetActive();
	void StatusbarSet(const Glib::ustring& text);

public:
	BackupWindow(BaseObjectType *cobject, const Glib::RefPtr<Gnome::Glade::Xml> &xml);
	~BackupWindow();

	// handler for exceptions that happen in signal calls
	void signal_exception_handler();

	// handler for treeview update requests
	void treeview_update();

	// signal handlers
	void on_backup();
	void on_restore();
	void on_config();
	void on_disconnect();
	void on_disconnect_all();
	void on_reload();
	void on_device_change();
	void on_file_quit();
	void on_help_about();
	bool on_startup();
};

