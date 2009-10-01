///
/// \file	Thread.cc
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

#include "Thread.h"
#include "util.h"

void Thread::SetStatus(std::string mode)
{
	m_status = mode;
	m_update->emit();
}

Thread::Thread(Device dev, Glib::Dispatcher *update_signal)
	: ConfigFile(dev.GetPIN())
	, m_dev(dev)
	, m_interface(&m_dev)
	, m_update(update_signal)
	, m_status("")
	, m_recordFinished(0)
	, m_recordTotal(0)
	, m_connected(false)
	, m_error(false)
	, m_thread_state(THREAD_STATE_IDLE)
{
	m_signal_progress.connect(
		sigc::mem_fun(*this, &Thread::on_thread_progress));
	m_signal_error.connect(
		sigc::mem_fun(*this, &Thread::on_thread_error));
	m_signal_done.connect(
		sigc::mem_fun(*this, &Thread::on_thread_done));
	m_signal_erase_db.connect(
		sigc::mem_fun(*this, &Thread::on_thread_erase_db));

	SetStatus("Ready");
}

void Thread::LoadConfig()
{
	ConfigFile::Load();
	if( m_connected )
		Enlighten(m_interface.GetDBDB());
	m_update->emit();
}

bool Thread::CheckFinishedMarker()
{
	if( !m_finished_marker )
		return false;
	m_finished_marker = false;
	return true;
}

std::string Thread::GetFullname()
{
	std::string ret = GetPIN().str() + " (" + GetDeviceName() + ")";
	return ret;
}

bool Thread::Connect()
{
	password_required = false;
	bad_size = false;
	try {
		if( !m_interface.Connect() )
			return (m_connected = false);
	}
	catch( Barry::BadPassword &bp ) {
		password_out_of_tries = bp.out_of_tries();
		password_remaining_tries = bp.remaining_tries();
		password_required = true;
		return (m_connected = false);
	}
	catch( Barry::BadSize &bs ) {
		bad_size_error = std::string("Barry::BadSize caught in Connect: ") + bs.what();
		bad_size = true;
		return (m_connected = false);
	}
	SetStatus("Connected");
	return (m_connected = true);
}

bool Thread::Connect(const std::string &password)
{
	try {
		m_interface.Password(password.c_str());
	}
	catch( Barry::BadPassword &bp ) {
		password_out_of_tries = bp.out_of_tries();
		password_remaining_tries = bp.remaining_tries();
		bad_password_error = bp.what();
		return (m_connected = false);
	}
	SetStatus("Connected");
	return (m_connected = true);
}

void Thread::Disconnect()
{
	if( m_connected )
	{
		m_interface.Disconnect();
		SetStatus("Ready");
		m_connected = false;
	}
}

void Thread::UnsetActive()
{
	m_active = false;
	if( !Working() )
		Disconnect();
}

bool Thread::Backup(std::string label)
{
	// only start a backup if currently idle
	if( Working() )
		return false;

	m_recordTotal = m_interface.GetRecordTotal(GetBackupList());
	m_recordFinished = 0;

	bool started = m_interface.StartBackup(
		DeviceInterface::AppComm(&m_signal_progress,
			&m_signal_error,
			&m_signal_done,
			&m_signal_erase_db),
		GetBackupList(), GetPath(), label);
	if( started ) {
		m_thread_state = THREAD_STATE_BACKUP;
		SetStatus("Backup");
	}
	return started;
}

bool Thread::Restore(std::string filename)
{
	// only start a restore if currently idle
	if( Working() )
		return false;

	m_recordTotal = m_interface.GetRecordTotal(GetRestoreList(), filename);
	m_recordFinished = 0;

	bool started = m_interface.StartRestore(
		DeviceInterface::AppComm(&m_signal_progress,
			&m_signal_error,
			&m_signal_done,
			&m_signal_erase_db),
		GetRestoreList(), filename);
	if( started ) {
		m_thread_state = THREAD_STATE_RESTORE;
		SetStatus("Restore");
	}
	return started;
}

bool Thread::RestoreAndBackup(std::string filename)
{
	// only start a restore if currently idle
	if( Working() )
		return false;

	// not supported
	return false;

/*
	m_working = m_interface.StartRestoreAndBackup(
		DeviceInterface::AppComm(&m_signal_progress,
			&m_signal_error,
			&m_signal_done,
			&m_signal_erase_db),
		GetRestoreList(), filename,
		GetPath());
	if( m_working )
		SetStatus("Restore & Backup");
	return m_working;
*/
}

void Thread::on_thread_progress()
{
	++m_recordFinished;
	m_update->emit();
}

void Thread::on_thread_error()
{
	m_error = true;
	m_thread_state |= THREAD_STATE_IDLE;

	Gtk::MessageDialog msg(m_status + " error: " + m_interface.get_last_thread_error());
	msg.run();
}

void Thread::on_thread_done()
{
	if( m_active )
		SetStatus("Connected");
	else
		Disconnect();
	m_thread_state |= THREAD_STATE_IDLE;
	m_finished_marker = true;
}

void Thread::on_thread_erase_db()
{
	std::string name = m_interface.GetThreadDBName();
	std::cerr << "Erasing database: " << name;
}

