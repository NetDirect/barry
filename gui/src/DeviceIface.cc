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
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <stdlib.h>

DeviceInterface::DeviceInterface()
	: m_con(0)
	, m_dbnameMutex(new Glib::Mutex) // this is just in an effort to
					 // avoid gtkmm headers in
					 // DeviceIface.h
	, m_end_of_tar(false)
	, m_tar_record_loaded(false)
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
	bool error = false;
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
		error = true;
	}
	catch( std::exception &e ) {
		m_last_thread_error = e.what();
		error = true;
	}
	catch( Quit &q ) {
		m_last_thread_error = "Terminated by user.";
	}

	m_tarback->Close();
	m_tarback.reset();

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
		while( !m_end_of_tar ) {
			try {
				// call the controller to do the work
				unsigned int dbId = m_con->GetDBID(m_current_dbname);
				m_AppComm.m_erase_db->emit();
				m_con->SaveDatabase(dbId, *this);
			}
			catch( Barry::Error &be ) {
				// save thread error
				m_last_thread_error = "Error while restoring ";
				m_last_thread_error += m_current_dbname + ".  ";
				m_last_thread_error += be.what();
				m_last_thread_error += "  Will continue processing.";

				// notify host thread
				m_AppComm.m_error->emit();

				// skip over records from this db
				std::cerr << "Error on database: "
					<< m_current_dbname << std::endl;
				SkipCurrentDB();
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
		m_last_thread_error = "Terminated by user.";
	}

	m_tar->Close();
	m_tar.reset();

	// signal host thread that we're done
	m_AppComm.m_done->emit();

	// done!
	m_AppComm.Invalidate();
}

void DeviceInterface::RestoreAndBackupThread()
{
	m_thread_quit = false;
	m_last_thread_error = "";

	try {

		// cycle until m_end_of_tar
		while( !m_end_of_tar ) {
			unsigned int dbId = m_con->GetDBID(m_current_dbname);

			try {

				// do restore first
				m_AppComm.m_erase_db->emit();
				m_con->SaveDatabase(dbId, *this);

			}
			catch( Barry::Error &be ) {
				// save thread error
				m_last_thread_error = "Error while restoring ";
				m_last_thread_error += m_current_dbname + ".  ";
				m_last_thread_error += be.what();
				m_last_thread_error += "  Will continue processing.";

				// notify host thread
				m_AppComm.m_error->emit();

				// skip over records from this db
				std::cerr << "Error on database: "
					<< m_current_dbname << std::endl;
				SkipCurrentDB();
			}

			// then the backup, even if restore fails
			m_con->LoadDatabase(dbId, *this);

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
		m_last_thread_error = "Terminated by user.";
	}

	m_tar->Close();
	m_tar.reset();
	m_tarback->Close();
	m_tarback.reset();

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
		<< "-"
		<< std::setw(2) << std::setfill('0') << lt->tm_hour
		<< std::setw(2) << std::setfill('0') << lt->tm_min
		<< std::setw(2) << std::setfill('0') << lt->tm_sec
		<< ".tar.gz";
	return tarfilename.str();
}

int DeviceInterface::CountFiles(reuse::TarFile &tar,
				const ConfigFile::DBListType &restoreList)
{
	int count = 0;
	std::string name, last_name;
	bool good = false;

	while( tar.ReadNextFilenameOnly(name) ) {
		std::string::size_type pos = name.rfind('/');
		if( pos == std::string::npos )
			continue;	// bad name
		std::string dbname = name.substr(0, pos);

		if( dbname != last_name ) {
			last_name = dbname;
			good = restoreList.IsSelected(dbname);
		}
		if( good )
			count++;
	}
	return count;
}

/// Splits a tarpath of the form "DBName/DBID" into separate string values.
/// Returns true if successful, false if tarpath is a bad name.
bool DeviceInterface::SplitTarPath(const std::string &tarpath,
				   std::string &dbname,
				   std::string &dbid_text,
				   uint8_t &dbrectype,
				   uint32_t &dbid)
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
		m_con->RetryPassword(password);
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

		std::string filename = directory + "/" + MakeFilename(pin);
		m_tarback.reset( new reuse::TarFile(filename.c_str(), true, &reuse::gztar_ops_nonthread, true) );

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
				   const std::string &filename,
				   int *pRecordCount)
{
	if( m_AppComm.IsValid() )
		return False("Thread already running.");

	try {
		if( pRecordCount ) {
			// caller is asking for a total, so we do a quick
			// scan through the tar file first
			m_tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );
			*pRecordCount = CountFiles(*m_tar, restoreList);

			// close for next open
			m_tar.reset();
		}

		// open for the main restore
		m_tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );

	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_AppComm = comm;
	m_dbList = restoreList;
	m_current_dbname_not_thread_safe = "";
	m_current_dbname = "";
	m_unique_id = 0;
	m_end_of_tar = false;
	m_tar_record_loaded = false;

	// get first tar record
	Retrieve(0);

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreThread), false);
	return true;
}

bool DeviceInterface::StartRestoreAndBackup(AppComm comm,
				const ConfigFile::DBListType &restoreAndBackupList,
				const std::string &filename,
				const std::string &directory, const std::string &pin,
				int *pRecordCount)
{
	if( m_AppComm.IsValid() )
		return False("Thread already running.");

	try {
		if( pRecordCount ) {
			// caller is asking for a total, so we do a quick
			// scan through the tar file first
			m_tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );
			*pRecordCount = CountFiles(*m_tar, restoreAndBackupList);

			// close for next open
			m_tar.reset();
		}

		// open for the main restore
		m_tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );

		// open for secondary backup
		std::string back = directory + "/" + MakeFilename(pin);
		m_tarback.reset( new reuse::TarFile(back.c_str(), true, &reuse::gztar_ops_nonthread, true) );

	}
	catch( reuse::TarFile::TarError &te ) {
		return False(te.what());
	}

	// setup
	m_AppComm = comm;
	m_dbList = restoreAndBackupList;
	m_current_dbname_not_thread_safe = "";
	m_current_dbname = "";
	m_unique_id = 0;
	m_end_of_tar = false;
	m_tar_record_loaded = false;

	// get first tar record
	Retrieve(0);

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreAndBackupThread), false);
	return true;
}



//////////////////////////////////////////////////////////////////////////////
// Barry::Parser overrides

void DeviceInterface::SetIds(uint8_t RecType, uint32_t UniqueId)
{
	m_rec_type = RecType;
	m_unique_id = UniqueId;
	std::ostringstream oss;
	oss << std::hex << m_unique_id << " " << (unsigned int)m_rec_type;
	m_tar_id_text = oss.str();
	if( m_tar_id_text.size() == 0 )
		throw std::runtime_error("No unique ID available!");
}

void DeviceInterface::ParseFields(const Barry::Data &data, size_t &offset)
{
	m_record_data.assign((const char*)data.GetData() + offset, data.GetSize() - offset);
}

void DeviceInterface::Store()
{
	std::string tarname = m_current_dbname + "/" + m_tar_id_text;
	m_tarback->AppendFile(tarname.c_str(), m_record_data);

	m_AppComm.m_progress->emit();

	// check quit flag
	if( m_thread_quit ) {
		throw Quit();
	}
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Builder overrides

bool DeviceInterface::Retrieve(unsigned int dbId)
{
	if( m_end_of_tar )
		return false;

	// if loaded, we are likely on a database
	// boundary, and the last read crossed it, so don't load again
	if( m_tar_record_loaded )
		return true;

	// search for a valid record
	for(;;) {
		// load record data from tar file
		std::string filename;
		if( !m_tar->ReadNextFile(filename, m_record_data) ) {
			// assume end of file
			m_end_of_tar = true;
			return false;
		}
		m_tar_record_loaded = true;

		// split record filename into dbname and ID
		std::string dbname;
		if( !SplitTarPath(filename, dbname, m_tar_id_text, m_rec_type, m_unique_id) ) {
			// invalid filename, skip it
			std::cerr << "Skipping invalid tar record: " << filename << std::endl;
			continue;
		}

		// are we working on the same dbname as last time? if so, go ahead!
		if( m_current_dbname == dbname ) {
			return true;
		}

		// DIFFERENT DBNAME from here on down!

		// does the filter allow this record?  if not, skip it and continue
		// looking
		if( !m_dbList.IsSelected(dbname) ) {
			continue;
		}

		// all checks pass, load the new dbname, and return false
		// if we are on a dbname boundary
		bool r_val = false;
		if( m_current_dbname.size() == 0 ) {
			// this is the first time through Retrieve, so ok
			r_val = true;
		}

		SetThreadDBName(dbname);
		return r_val;
	}
}

uint8_t DeviceInterface::GetRecType() const
{
	return m_rec_type;
}

uint32_t DeviceInterface::GetUniqueId() const
{
	return m_unique_id;
}

void DeviceInterface::BuildHeader(Barry::Data &data, size_t &offset)
{
	// nothing to do
}

void DeviceInterface::BuildFields(Barry::Data &data, size_t &offset)
{
	int packet_size = offset + m_record_data.size();
	unsigned char *buf = data.GetBuffer(packet_size);
	memcpy(buf + offset, m_record_data.data(), m_record_data.size());
	offset += m_record_data.size();
	data.ReleaseBuffer(packet_size);

	// clear loaded flag, as it has now been used
	m_tar_record_loaded = false;

	m_AppComm.m_progress->emit();
}

// helper function for halding restore errors
void DeviceInterface::SkipCurrentDB() throw()
{
	// skip all records until next DB
	try {
		while( Retrieve(0) ) {
			std::cerr << "Skipping: "
				<< m_current_dbname << "/"
				<< m_tar_id_text << std::endl;
			m_tar_record_loaded = false;
		}
	}
	catch( reuse::TarFile::TarError & ) {
		m_end_of_tar = true;
	}
	catch( ... ) {
		// swallow all other exceptions
		std::cerr << "EXCEPTION IN SkipCurrentDB()!  "
			"Please report to Barry mailing list." << std::endl;
	}
}

