///
/// \file	Thread.h
///		Thread class for device manipulation
///

/*
    Copyright (C) 2007-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2009, Ryan Li (ryan@ryanium.com)

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

#ifndef __BARRYBACKUP_THREAD_H__
#define __BARRYBACKUP_THREAD_H__

#include <gtkmm.h>
#include "ConfigFile.h"
#include "DeviceIface.h"

class Thread : public ConfigFile
{
	Glib::Dispatcher m_signal_progress;
	Glib::Dispatcher m_signal_error;
	Glib::Dispatcher m_signal_done;
	Glib::Dispatcher m_signal_erase_db;

	Device m_dev;
	DeviceInterface m_interface;

	Glib::Dispatcher *m_update;

	std::string m_status;

	unsigned int m_recordFinished;
	unsigned int m_recordTotal;

	// Barry::BadPassword related variables
	bool password_out_of_tries;
	unsigned int password_remaining_tries;
	bool password_required;
	std::string bad_password_error;

	// Barry::BadSize related variables
	bool bad_size;
	std::string bad_size_error;

	// whether the device is active in Gtk::TreeView
	bool m_active;

	// marked as true after thread finished,
	// and false when the value is retrieved.
	bool m_finished_marker;

	// states
	bool m_connected;
	bool m_working;
	bool m_error;

protected:
	void SetStatus(std::string);

public:
	Thread(Device, Glib::Dispatcher *);
	~Thread() {}

	std::string LastInterfaceError() { return m_interface.get_last_error(); }
	std::string LastConfigError() { return ConfigFile::get_last_error(); }

	const Barry::DatabaseDatabase &GetDBDB() { return m_interface.GetDBDB(); }

	void LoadConfig();

	Pin GetPIN() { return m_dev.GetPIN(); }
	std::string GetFullname();

	std::string Status() const { return m_status; }
	bool CheckFinishedMarker();
	unsigned int GetRecordFinished() const { return m_recordFinished; }
	unsigned int GetRecordTotal() const { return m_recordTotal; }

	void Reset() { m_interface.Reset(); }
	bool Connect();
	bool Connect(const std::string &password);
	void Disconnect();

	void SetActive() { m_active = true; }
	void UnsetActive();

	bool Connected() const { return m_connected; }
	bool Working() const { return m_working; }
	bool Error() const { return m_error; }

	bool PasswordRequired() const { return password_required; }
	bool PasswordOutOfTries() const { return password_out_of_tries; }
	unsigned int PasswordRemainingTries() const { return password_remaining_tries; }
	std::string BadPasswordError() const { return bad_password_error; }

	bool BadSize() const { return bad_size; }
	std::string BadSizeError() const { return bad_size_error; }

	bool Backup(std::string label);
	bool Restore(std::string filename);
	bool RestoreAndBackup(std::string filename);

	void on_thread_progress();
	void on_thread_error();
	void on_thread_done();
	void on_thread_erase_db();
};

#endif

