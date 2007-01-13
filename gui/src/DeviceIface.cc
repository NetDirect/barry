///
/// \file	DeviceIface.cc
///		Interface class for device backup and restore
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

#include "DeviceIface.h"
#include <glibmm.h>
#include <sstream>

DeviceInterface::DeviceInterface()
	: m_con(0)
	, m_signal_progress(0)
	, m_signal_done(0)
	, m_thread_quit(false)
{
}

DeviceInterface::~DeviceInterface()
{
	delete m_con;
}

bool DeviceInterface::False(const std::string &msg)
{
	m_last_error = msg;
	return false;
}

void DeviceInterface::BackupThread()
{
	m_thread_quit = false;
	m_last_thread_error = "";

	try {

		// cycle through all database names in the dbList
		// and store them all
		ConfigFile::DBListType::const_iterator name = m_dbList.begin();
		for( ; name != m_dbList.end(); ++name ) {
			// save current db name
			m_current_dbname = *name;

			// call the controller to do the work
			unsigned int dbId = m_con->GetDBID(m_current_dbname);
			m_con->LoadDatabase(dbId, *this);
		}

	}
	catch( Glib::Exception &e ) {
		m_last_thread_error = e.what();
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
	}

	m_tar->Close();
	m_tar.reset();

	// signal host thread that we're done
	m_signal_done->emit();

	// done!
	m_signal_progress = m_signal_done = 0;
}

void DeviceInterface::RestoreThread()
{
	m_thread_quit = false;
	m_last_thread_error = "";

	try {
	}
	catch( Glib::Exception &e ) {
		m_last_thread_error = e.what();
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
	}

	m_tar->Close();
	m_tar.reset();

	// signal host thread that we're done
	m_signal_done->emit();

	// done!
	m_signal_progress = m_signal_done = 0;
}


//////////////////////////////////////////////////////////////////////////////
// Public API

bool DeviceInterface::Connect(const Barry::ProbeResult &dev)
{
	try {
		Disconnect();
		m_con = new Barry::Controller(dev);
		m_con->OpenMode(Barry::Controller::Desktop);
		return true;
	}
	catch( Barry::BError &e ) {
		m_con = 0;
		return False(e.what());
	}
}

void DeviceInterface::Disconnect()
{
	delete m_con;
	m_con = 0;
}

bool DeviceInterface::StartBackup(Glib::Dispatcher *progress,
				  Glib::Dispatcher *done,
				  const ConfigFile::DBListType &backupList,
				  const std::string &filename)
{
	if( m_signal_progress || m_signal_done )
		return False("Thread already running.");

	try {
		m_tar.reset( new reuse::TarFile(filename.c_str(), true, true, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_signal_progress = progress;
	m_signal_done = done;
	m_dbList = backupList;

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::BackupThread), false);
	return true;
}

bool DeviceInterface::StartRestore(Glib::Dispatcher *progress,
				   Glib::Dispatcher *done,
				   const ConfigFile::DBListType &restoreList,
				   const std::string &filename)
{
	if( m_signal_progress || m_signal_done )
		return False("Thread already running.");

	try {
		m_tar.reset( new reuse::TarFile(filename.c_str(), false, true, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_signal_progress = progress;
	m_signal_done = done;
	m_dbList = restoreList;

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreThread), false);
	return true;
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Parser overrides

void DeviceInterface::SetUniqueId(uint32_t Id)
{
	m_unique_id = Id;
	std::ostringstream oss;
	oss << std::hex << m_unique_id;
	m_unique_id_text = oss.str();
	if( m_unique_id_text.size() == 0 )
		throw std::runtime_error("No unique ID available!");
}

void DeviceInterface::ParseFields(const Data &data, size_t &offset)
{
	m_record_data.assign((const char*)data.GetData(), data.GetSize() - offset);
}

void DeviceInterface::Store()
{
	std::string tarname = m_current_dbname + "/" + m_unique_id_text;
	m_tar->AppendFile(tarname.c_str(), m_record_data);

	m_signal_progress->emit();
}


