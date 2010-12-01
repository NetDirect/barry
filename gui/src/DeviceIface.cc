///
/// \file	DeviceIface.cc
///		Interface class for device backup and restore
///

/*
    Copyright (C) 2007-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

DeviceInterface::DeviceInterface(Device *dev)
	: m_dev(dev)
	, m_con(0)
	, m_desktop(0)
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
		Barry::ConfigFile::DBListType::const_iterator name = m_dbList.begin();
		for( ; name != m_dbList.end(); ++name ) {
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
				unsigned int dbId = m_desktop->GetDBID(m_current_dbname);
				m_AppComm.m_erase_db->emit();
				m_desktop->SaveDatabase(dbId, *this);
				m_AppComm.m_restored_db->emit();
			}
			catch( Barry::Error &be ) {
				// save thread error
				m_last_thread_error = _("Error while restoring ");
				m_last_thread_error += m_current_dbname + ".  ";
				m_last_thread_error += be.what();
				m_last_thread_error += _("  Will continue processing.");

				// notify host thread
				m_AppComm.m_error->emit();

				// skip over records from this db
				std::cerr << _("Error on database: ")
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
		m_last_thread_error = _("Terminated by user.");
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
			unsigned int dbId = m_desktop->GetDBID(m_current_dbname);

			try {

				// do restore first
				m_AppComm.m_erase_db->emit();
				m_desktop->SaveDatabase(dbId, *this);

			}
			catch( Barry::Error &be ) {
				// save thread error
				m_last_thread_error = _("Error while restoring ");
				m_last_thread_error += m_current_dbname + ".  ";
				m_last_thread_error += be.what();
				m_last_thread_error += _("  Will continue processing.");

				// notify host thread
				m_AppComm.m_error->emit();

				// skip over records from this db
				std::cerr << _("Error on database: ")
					<< m_current_dbname << std::endl;
				SkipCurrentDB();
			}

			// then the backup, even if restore fails
			m_desktop->LoadDatabase(dbId, *this);

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

	m_tar->Close();
	m_tar.reset();
	m_tarback->Close();
	m_tarback.reset();

	// signal host thread that we're done
	m_AppComm.m_done->emit();

	// done!
	m_AppComm.Invalidate();
}

std::string DeviceInterface::MakeFilename(const std::string &label) const
{
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	std::string fileLabel = label;
	if( fileLabel.size() ) {
		// prepend a hyphen
		fileLabel.insert(fileLabel.begin(), '-');

		// change all spaces in label to underscores
		for( size_t i = 0; i < fileLabel.size(); i++ ) {
			if( fileLabel[i] == ' ' )
				fileLabel[i] = '_';
		}
	}

	std::ostringstream tarfilename;
	tarfilename << m_dev->GetPIN().str() << "-"
		<< std::setw(4) << std::setfill('0') << (lt->tm_year + 1900)
		<< std::setw(2) << std::setfill('0') << (lt->tm_mon + 1)
		<< std::setw(2) << std::setfill('0') << lt->tm_mday
		<< "-"
		<< std::setw(2) << std::setfill('0') << lt->tm_hour
		<< std::setw(2) << std::setfill('0') << lt->tm_min
		<< std::setw(2) << std::setfill('0') << lt->tm_sec
		<< fileLabel
		<< ".tar.gz";
	return tarfilename.str();
}

int DeviceInterface::CountFiles(reuse::TarFile &tar,
				const Barry::ConfigFile::DBListType &restoreList) const
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
	m_current_dbname_not_thread_safe = dbname;
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
	unsigned int count = 0;

	std::auto_ptr<reuse::TarFile> tar;

	try {
		// do a scan through the tar file
		tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );
		count = CountFiles(*tar, restoreList);
	}
	catch( reuse::TarFile::TarError &te ) {
		// just throw it away
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
				  const Barry::ConfigFile::DBListType &backupList,
				  const std::string &directory,
				  const std::string &backupLabel)
{
	if( m_AppComm.IsValid() )
		return False(_("Thread already running."));

	try {
		std::string filename = directory + "/" + MakeFilename(backupLabel);
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
				   const Barry::ConfigFile::DBListType &restoreList,
				   const std::string &filename)
{
	if( m_AppComm.IsValid() )
		return False(_("Thread already running."));

	try {
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
	Retrieve();

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreThread), false);
	return true;
}

bool DeviceInterface::StartRestoreAndBackup(AppComm comm,
				const Barry::ConfigFile::DBListType &restoreAndBackupList,
				const std::string &filename,
				const std::string &directory)
{
	if( m_AppComm.IsValid() )
		return False(_("Thread already running."));

	try {
		// open for the main restore
		m_tar.reset( new reuse::TarFile(filename.c_str(), false, &reuse::gztar_ops_nonthread, true) );

		// open for secondary backup
		std::string back = directory + "/" + MakeFilename(m_dev->GetPIN().str());
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
	Retrieve();

	// start the thread
	Glib::Thread::create(sigc::mem_fun(*this, &DeviceInterface::RestoreAndBackupThread), false);
	return true;
}



//////////////////////////////////////////////////////////////////////////////
// Barry::Parser overrides

void DeviceInterface::ParseRecord(const Barry::DBData &data,
				  const Barry::IConverter *ic)
{
	m_rec_type = data.GetRecType();
	m_unique_id = data.GetUniqueId();
	std::ostringstream oss;
	oss << std::hex << m_unique_id << " " << (unsigned int)m_rec_type;
	m_tar_id_text = oss.str();
	if( m_tar_id_text.size() == 0 )
		throw std::runtime_error(_("No unique ID available!"));

	m_record_data.assign(
		(const char*)data.GetData().GetData() + data.GetOffset(),
		data.GetData().GetSize() - data.GetOffset());

	// store in tarball
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

bool DeviceInterface::Retrieve()
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
			std::cerr << _("Skipping invalid tar record: ") << filename << std::endl;
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

bool DeviceInterface::BuildRecord(Barry::DBData &data, size_t &offset,
				  const Barry::IConverter *ic)
{
	if( !Retrieve() )
		return false;

	// fill in the meta data
	data.SetVersion(Barry::DBData::REC_VERSION_1);
	data.SetDBName(m_current_dbname);
	data.SetIds(m_rec_type, m_unique_id);
	data.SetOffset(offset);

	// fill in the record data
	int packet_size = offset + m_record_data.size();
	Barry::Data &block = data.UseData();
	unsigned char *buf = block.GetBuffer(packet_size);
	memcpy(buf + offset, m_record_data.data(), m_record_data.size());
	offset += m_record_data.size();
	block.ReleaseBuffer(packet_size);

	// clear loaded flag, as it has now been used
	m_tar_record_loaded = false;
	m_AppComm.m_progress->emit();
	return true;
}

bool DeviceInterface::FetchRecord(Barry::DBData &data,
				  const Barry::IConverter *ic)
{
	size_t offset = 0;
	return BuildRecord(data, offset, ic);
}

// helper function for halding restore errors
void DeviceInterface::SkipCurrentDB() throw()
{
	// skip all records until next DB
	try {
		while( Retrieve() ) {
			std::cerr << _("Skipping: ")
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

Device::Device(const Barry::ProbeResult &result)
	: result(result)
{
}

