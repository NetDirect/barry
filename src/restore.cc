///
/// \file	restore.cc
///		Builder class for restoring from Barry Backup files
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "restore.h"
#include "tarfile.h"
#include "error.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string.h>

namespace Barry {

namespace {

	int CountFiles(reuse::TarFile &tar,
			const Barry::Restore::DBListType &restoreList,
			bool default_all_db)
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
				good = (default_all_db && restoreList.size() == 0) ||
					restoreList.IsSelected(dbname);
			}
			if( good )
				count++;
		}
		return count;
	}

}

//////////////////////////////////////////////////////////////////////////////
// Static Restore members

/// Splits a tarpath of the form "DBName/DBID" into separate string values.
/// Returns true if successful, false if tarpath is a bad name.
bool Restore::SplitTarPath(const std::string &tarpath,
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


//////////////////////////////////////////////////////////////////////////////
// Restore - constructors

Restore::Restore(const std::string &tarpath, bool default_all_db)
	: m_default_all_db(default_all_db)
	, m_end_of_tar(false)
	, m_tar_record_loaded(false)
	, m_rec_type(0)
	, m_unique_id(0)
{
	try {
		m_tar.reset( new reuse::TarFile(tarpath.c_str(), false,
					&reuse::gztar_ops_nonthread, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::RestoreError(te.what());
	}
}

Restore::~Restore()
{
}


//////////////////////////////////////////////////////////////////////////////
// Restore - Protected helpers

bool Restore::IsSelected(const std::string &dbName) const
{
	// if nothing is in the list, default to all
	if( m_dbList.size() == 0 )
		return true;
	else
		return m_dbList.IsSelected(dbName);
}


//////////////////////////////////////////////////////////////////////////////
// Restore - Public API

void Restore::AddDB(const std::string &dbName)
{
	m_dbList.push_back(dbName);
}

void Restore::SkipCurrentDB()
{
	// skip all records until next DB
	try {
		while( Retrieve() ) {
			std::cerr << "Skipping: "
				<< m_current_dbname << "/"
				<< m_tar_id_text << std::endl;
			m_tar_record_loaded = false;
		}
	}
	catch( reuse::TarFile::TarError & ) {
		m_end_of_tar = true;
	}
}

unsigned int Restore::GetRecordTotal(const std::string &tarpath) const
{
	unsigned int count = 0;

	std::auto_ptr<reuse::TarFile> tar;

	try {
		// do a scan through the tar file
		tar.reset( new reuse::TarFile(tarpath.c_str(), false,
				&reuse::gztar_ops_nonthread, true) );
		count = CountFiles(*tar, m_dbList, m_default_all_db);
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::RestoreError(te.what());
	}
	return count;
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Builder overrides

bool Restore::Retrieve()
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

		// are we working on the same dbname as last time?
		// if so, go ahead!
		if( m_current_dbname == dbname ) {
			return true;
		}

		// DIFFERENT DBNAME from here on down!

		// does the filter allow this record?
		// if not, skip it and continue looking
		if( !IsSelected(dbname) ) {
			m_tar_record_loaded = false;
			continue;
		}

		// all checks pass, load the new dbname, and return false
		// if we are on a dbname boundary
		bool r_val = false;
		if( m_current_dbname.size() == 0 ) {
			// this is the first time through Retrieve, so ok
			r_val = true;
		}

		m_current_dbname = dbname;
		return r_val;
	}
}

std::string Restore::GetDBName() const
{
	return m_current_dbname;
}

uint8_t Restore::GetRecType() const
{
	return m_rec_type;
}

uint32_t Restore::GetUniqueId() const
{
	return m_unique_id;
}

void Restore::BuildHeader(Barry::Data &data, size_t &offset)
{
	// nothing to do
}

void Restore::BuildFields(Barry::Data &data, size_t &offset,
				  const Barry::IConverter *ic)
{
	int packet_size = offset + m_record_data.size();
	unsigned char *buf = data.GetBuffer(packet_size);
	memcpy(buf + offset, m_record_data.data(), m_record_data.size());
	offset += m_record_data.size();
	data.ReleaseBuffer(packet_size);
}

void Restore::BuildDone()
{
	// clear loaded flag, as it has now been used
	m_tar_record_loaded = false;
}

} // namespace Barry

