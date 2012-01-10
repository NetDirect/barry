///
/// \file	DeviceIface.cc
///		Interface class for device backup and restore
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

#include "DeviceIface.h"
#include "util.h"
#include "i18n.h"
#include <glibmm.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

DeviceInterface::DeviceInterface(Device *dev)
	: m_dev(dev)
	, m_con(0)
	, m_desktop(0)
	, m_dbnameMutex(new Glib::Mutex) // this is just in an effort to
					 // avoid gtkmm headers in
					 // DeviceIface.h
	, m_thread_quit(false)
{
}

DeviceInterface::~DeviceInterface()
{
	Disconnect();
	delete m_dbnameMutex;
}

bool DeviceInterface::False(const std::string &msg)
{
	m_last_error = msg;
	return false;
}

void DeviceInterface::BackupThread()
{
	bool error = false;
	m_thread_quit = false;
	m_last_thread_error = "";

	try {
		// cycle through all database names in the dbList
		// and store them all
		Barry::ConfigFile::DBListType::const_iterator name = m_dbBackupList.begin();
		for( ; name != m_dbBackupList.end(); ++name ) {
			// save current db name
			SetThreadDBName(*name);
			// call the controller to do the work
			unsigned int dbId = m_desktop->GetDBID(*name);
			m_desktop->LoadDatabase(dbId, *this);
		}
	}
	catch( Glib::Exception &e ) {
		m_last_thread_error = e.what();
		error = true;
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
		error = true;
	}
	catch( Quit &q ) {
		m_last_thread_error = _("Terminated by user.");
	}

	m_backupStats = m_backup->GetStats();
	m_backup.reset();

	if( error )
		m_AppComm.m_error->emit();

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

		// cycle until m_end_of_tar
		Barry::DBData meta;
		while( !m_restore->EndOfFile() ) {
			try {
				if( m_restore->GetNextMeta(meta) ) {
					// save current db name
					SetThreadDBName(meta.GetDBName());
					// call the controller to do the work
					unsigned int dbId = m_desktop->GetDBID(meta.GetDBName());
					m_AppComm.m_erase_db->emit();
					m_desktop->SaveDatabase(dbId, *this);
					m_AppComm.m_restored_db->emit();
				}
			}
			catch( Barry::Error &be ) {
				// save thread error
				m_last_thread_error = _("Error while restoring ");
				m_last_thread_error += meta.GetDBName() + ".  ";
				m_last_thread_error += be.what();
				m_last_thread_error += _("  Will continue processing.");

				// notify host thread
				m_AppComm.m_error->emit();

				// skip over records from this db
				std::cerr << _("Error on database: ")
					<< meta.GetDBName()
					<< " (" << be.what() << ")"
					<< std::endl;
				m_restore->SkipCurrentDB();
			}
		}

	}
	catch( Glib::Exception &e ) {
		m_last_thread_error = e.what();
		m_AppComm.m_error->emit();
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
		m_AppComm.m_error->emit();
	}
	catch( Quit &q ) {
		m_last_thread_error = _("Terminated by user.");
	}

	m_restore.reset();

	// signal host thread that we're done
	m_AppComm.m_done->emit();

	// done!
	m_AppComm.Invalidate();
}

std::string DeviceInterface::MakeFilename(const std::string &label) const
{
	return Barry::MakeBackupFilename(m_dev->GetPIN(), label);
}

/// Splits a tarpath of the form "DBName/DBID" into separate string values.
/// Returns true if successful, false if tarpath is a bad name.
bool DeviceInterface::SplitTarPath(const std::string &tarpath,
				   std::string &dbname,
				   std::string &dbid_text,
				   uint8_t &dbrectype,
				   uint32_t &dbid) const
{
	std::string::size_type pos = tarpath.rfind('/');
	if( pos == std::string::npos )
		return false;		// bad name

	dbname = tarpath.substr(0, pos);
	dbid_text = tarpath.substr(pos + 1);
	if( dbname.size() == 0 || dbid_text.size() == 0 )
		return false;		// bad name

	std::istringstream iss(dbid_text);
	unsigned int temp;
	iss >> std::hex >> dbid >> temp;
	dbrectype = (uint8_t) temp;

	return true;
}

void DeviceInterface::SetThreadDBName(const std::string &dbname)
{
	Glib::Mutex::Lock lock(*m_dbnameMutex);
	m_current_dbname = dbname;
}

//////////////////////////////////////////////////////////////////////////////
// Public API

void DeviceInterface::Reset()
{
	Usb::Device dev(m_dev->result.m_dev);
	dev.Reset();
}

bool DeviceInterface::Connect()
{
	try {
		Disconnect();
		m_con = new Barry::Controller(m_dev->result);
		m_desktop = new Barry::Mode::Desktop(*m_con);
		m_desktop->Open();
		return true;
	}
	catch( Barry::BadPassword & ) {
		// pass on to the caller
		throw;
	}
	catch( Barry::BadSize & ) {
		// pass on to the caller
		Disconnect();
		throw;
	}
	catch( Barry::Error &e ) {
		Disconnect();
		return False(e.what());
	}
}

bool DeviceInterface::Password(const char *password)
{
	try {
		m_desktop->RetryPassword(password);
		return true;
	}
	catch( Barry::BadPassword & ) {
		// pass on to the caller
		throw;
	}
	catch( Barry::Error &e ) {
		Disconnect();
		return False(e.what());
	}
}

void DeviceInterface::Disconnect()
{
	delete m_desktop;
	m_desktop = 0;

	delete m_con;
	m_con = 0;
}

// cycle through controller's DBDB and count the records in all the
// databases selected in the backupList
unsigned int DeviceInterface::GetRecordTotal(const Barry::ConfigFile::DBListType &backupList) const
{
	unsigned int count = 0;

	Barry::DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_desktop->GetDBDB().Databases.begin();
	for( ; i != m_desktop->GetDBDB().Databases.end(); ++i ) {
		if( backupList.IsSelected(i->Name) ) {
			count += i->RecordCount;
		}
	}
	return count;
}

unsigned int DeviceInterface::GetRecordTotal(const Barry::ConfigFile::DBListType &restoreList, const std::string &filename) const
{
	return Barry::Restore::GetRecordTotal(filename, restoreList, false);
}

// cycle through controller's DBDB and compare the record count of each
// database, and store an error message for each one that is not equal,
// and return the list of messages as a vector
std::vector<std::string> DeviceInterface::CompareTotals(const Barry::ConfigFile::DBListType &backupList) const
{
	vector<string> msgs;

	Barry::DatabaseDatabase::DatabaseArrayType::const_iterator
		i = m_desktop->GetDBDB().Databases.begin();
	for( ; i != m_desktop->GetDBDB().Databases.end(); ++i ) {
		if( backupList.IsSelected(i->Name) ) {
			if( (int)i->RecordCount != m_backupStats[i->Name] ) {
				ostringstream oss;
				oss << "'" << i->Name << "' claimed to have " << i->RecordCount << " records, but actually retrieved " << m_backupStats[i->Name] << ".";
				msgs.push_back(oss.str());
			}
		}
	}

	return msgs;
}

/// returns name of database the thread is currently working on
std::string DeviceInterface::GetThreadDBName() const
{
	Glib::Mutex::Lock lock(*m_dbnameMutex);
	return m_current_dbname;
}

bool DeviceInterface::StartBackup(AppComm comm,
				  const Barry::ConfigFile::DBListType &backupList,
				  const std::string &directory,
				  const std::string &backupLabel)
{
	if( m_AppComm.IsValid() )
		return False(_("Thread already running."));

	try {
		std::string filename = directory + "/" + MakeFilename(backupLabel);
		m_backup.reset( new Barry::Backup(filename.c_str()) );
	}
	catch( Barry::BackupError &be ) {
		return False(be.what());
	}

	// setup
	m_AppComm = comm;
	m_dbBackupList = backupList;
	SetThreadDBName("");

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::BackupThread), false);
	return true;
}

bool DeviceInterface::StartRestore(AppComm comm,
				   const Barry::ConfigFile::DBListType &restoreList,
				   const std::string &filename)
{
	if( m_AppComm.IsValid() )
		return False(_("Thread already running."));

	try {
		m_restore.reset( new Barry::Restore(filename.c_str(), false) );
	}
	catch( Barry::BackupError &be ) {
		return False(be.what());
	}

	// setup
	m_AppComm = comm;
	m_restore->Add(restoreList);
	SetThreadDBName("");

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreThread), false);
	return true;
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Parser overrides

void DeviceInterface::ParseRecord(const Barry::DBData &data,
				  const Barry::IConverter *ic)
{
	m_backup->ParseRecord(data, ic);
	m_AppComm.m_progress->emit();

	// check quit flag
	if( m_thread_quit ) {
		throw Quit();
	}
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Builder overrides

bool DeviceInterface::BuildRecord(Barry::DBData &data, size_t &offset,
				  const Barry::IConverter *ic)
{
	// check quit flag
	if( m_thread_quit ) {
		throw Quit();
	}

	if( m_restore->BuildRecord(data, offset, ic) ) {
		m_AppComm.m_progress->emit();
		return true;
	}
	else {
		return false;
	}
}

bool DeviceInterface::FetchRecord(Barry::DBData &data,
				  const Barry::IConverter *ic)
{
	size_t offset = 0;
	return BuildRecord(data, offset, ic);
}

Device::Device(const Barry::ProbeResult &result)
	: result(result)
{
}

