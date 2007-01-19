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
#include "util.h"
#include <glibmm.h>
#include <iomanip>
#include <sstream>
#include <time.h>

DeviceInterface::DeviceInterface()
	: m_con(0)
	, m_dbnameMutex(new Glib::Mutex) // this is just in an effort to
					 // avoid gtkmm headers in
					 // DeviceIface.h
	, m_thread_quit(false)
{
}

DeviceInterface::~DeviceInterface()
{
	delete m_con;
	delete m_dbnameMutex;
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
			SetThreadDBName(*name);

			// call the controller to do the work
			unsigned int dbId = m_con->GetDBID(*name);
			m_con->LoadDatabase(dbId, *this);
		}

	}
	catch( Glib::Exception &e ) {
		m_last_thread_error = e.what();
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
	}
	catch( Quit &q ) {
		m_last_thread_error = "Terminated by user.";
	}

	m_tar->Close();
	m_tar.reset();

	// signal host thread that we're done
	m_AppComm.m_done->emit();

	// done!
	m_AppComm.Invalidate();
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
	catch( Quit &q ) {
		m_last_thread_error = "Terminated by user.";
	}

	m_tar->Close();
	m_tar.reset();

	// signal host thread that we're done
	m_AppComm.m_done->emit();

	// done!
	m_AppComm.Invalidate();
}

std::string DeviceInterface::MakeFilename(const std::string &pin)
{
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	std::ostringstream tarfilename;
	tarfilename << pin << "-"
		<< std::setw(4) << std::setfill('0') << (lt->tm_year + 1900)
		<< std::setw(2) << std::setfill('0') << (lt->tm_mon + 1)
		<< std::setw(2) << std::setfill('0') << lt->tm_mday
		<< std::setw(2) << std::setfill('0') << lt->tm_hour
		<< std::setw(2) << std::setfill('0') << lt->tm_min
		<< std::setw(2) << std::setfill('0') << lt->tm_sec
		<< ".tar.gz";
	return tarfilename.str();
}

void DeviceInterface::SetThreadDBName(const std::string &dbname)
{
	Glib::Mutex::Lock lock(*m_dbnameMutex);
	m_current_dbname_not_thread_safe = dbname;
	m_current_dbname = dbname;
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

// cycle through controller's DBDB and count the records in all the
// databases selected in the backupList
int DeviceInterface::GetDeviceRecordTotal(const ConfigFile::DBListType &backupList) const
{
	int count = 0;

	Barry::DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_con->GetDBDB().Databases.begin();
	for( ; i != m_con->GetDBDB().Databases.end(); ++i ) {
		if( backupList.IsSelected(i->Name) ) {
			count += i->RecordCount;
		}
	}
	return count;
}

/// returns name of database the thread is currently working on
std::string DeviceInterface::GetThreadDBName() const
{
	Glib::Mutex::Lock lock(*m_dbnameMutex);
	return m_current_dbname_not_thread_safe;
}

bool DeviceInterface::StartBackup(AppComm comm,
				  const ConfigFile::DBListType &backupList,
				  const std::string &directory,
				  const std::string &pin)
{
	if( m_AppComm.IsValid() )
		return False("Thread already running.");

	try {
// fixme - get record total here
		std::string filename = directory + "/" + MakeFilename(pin);
		m_tar.reset( new reuse::TarFile(filename.c_str(), true, true, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_AppComm = comm;
	m_dbList = backupList;

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::BackupThread), false);
	return true;
}

bool DeviceInterface::StartRestore(AppComm comm,
				   const ConfigFile::DBListType &restoreList,
				   const std::string &directory,
				   const std::string &pin)
{
	if( m_AppComm.IsValid() )
		return False("Thread already running.");

	try {
		std::string filename = directory + "/" + MakeFilename(pin);
		m_tar.reset( new reuse::TarFile(filename.c_str(), false, true, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_AppComm = comm;
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

void DeviceInterface::ParseFields(const Barry::Data &data, size_t &offset)
{
	m_record_data.assign((const char*)data.GetData(), data.GetSize() - offset);
}

void DeviceInterface::Store()
{
	std::string tarname = m_current_dbname + "/" + m_unique_id_text;
	m_tar->AppendFile(tarname.c_str(), m_record_data);

	m_AppComm.m_progress->emit();

	// check quit flag
	if( m_thread_quit ) {
		throw Quit();
	}
}


