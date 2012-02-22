///
/// \file	restore.cc
///		Builder class for restoring from Barry Backup files
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <algorithm>

using namespace std;

namespace Barry {

namespace {

	int CountFiles(reuse::TarFile &tar,
			const Barry::Restore::DBListType &restoreList,
			Barry::Restore::DBListType *available,
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

				if( good && available )
					available->push_back(dbname);
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
	: m_tarpath(tarpath)
	, m_default_all_db(default_all_db)
	, m_tar_record_state(RS_EMPTY)
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
	// if in skip list, always return false,
	// if nothing is in the main list, use default
	// otherwise, only return true if specifically selected
	if( m_dbSkipList.IsSelected(dbName) )
		return false;
	else if( m_dbList.size() == 0 )
		return m_default_all_db;
	else
		return m_dbList.IsSelected(dbName);
}


//////////////////////////////////////////////////////////////////////////////
// Restore - Public API

void Restore::AddDB(const std::string &dbName)
{
	if( find(m_dbList.begin(), m_dbList.end(), dbName) == m_dbList.end() ) {
		// only add it if it is not already in the list
		m_dbList.push_back(dbName);
	}
}

void Restore::Add(const DBListType &dbList)
{
	for( DBListType::const_iterator i = dbList.begin();
		i != dbList.end();
		++i )
	{
		AddDB(*i);
	}
}

void Restore::Add(const DatabaseDatabase &dbdb)
{
	for( DatabaseDatabase::DatabaseArrayType::const_iterator i = dbdb.Databases.begin();
		i != dbdb.Databases.end();
		++i )
	{
		AddDB(i->Name);
	}
}

void Restore::AddSkipDB(const std::string &dbName)
{
	if( find(m_dbSkipList.begin(), m_dbSkipList.end(), dbName) == m_dbSkipList.end() ) {
		// only add it if it is not already in the list
		m_dbSkipList.push_back(dbName);
	}
}

void Restore::SkipCurrentDB()
{
	// skip all records until next DB
	try {
		Restore::RetrievalState state;
		while( (state = Retrieve(m_record_data)) == RS_NEXT ) {
			std::cerr << "Skipping: "
				<< m_current_dbname << "/"
				<< m_tar_id_text << std::endl;
			m_tar_record_state = RS_EMPTY;
		}

		if( state == RS_DBEND ) {
			// process the end of database, so that user is free
			// to call GetNextMeta() or Retrieve() or BuildRecord()
			m_tar_record_state = RS_NEXT;
		}
	}
	catch( reuse::TarFile::TarError & ) {
		m_tar_record_state = RS_EOF;
	}
}

unsigned int Restore::GetRecordTotal() const
{
	return GetRecordTotal(m_tarpath, m_dbList, m_default_all_db);
}

unsigned int Restore::GetRecordTotal(const std::string &tarpath,
					const DBListType &dbList,
					bool default_all_db)
{
	unsigned int count = 0;

	std::auto_ptr<reuse::TarFile> tar;

	try {
		// do a scan through the tar file
		tar.reset( new reuse::TarFile(tarpath.c_str(), false,
				&reuse::gztar_ops_nonthread, true) );
		count = CountFiles(*tar, dbList, 0, default_all_db);
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::RestoreError(te.what());
	}
	return count;
}

Barry::Restore::DBListType Restore::GetDBList() const
{
	return GetDBList(m_tarpath);
}

Barry::Restore::DBListType Restore::GetDBList(const std::string &tarpath)
{
	unsigned int count = 0;

	std::auto_ptr<reuse::TarFile> tar;
	DBListType available, empty;

	try {
		// do a scan through the tar file
		tar.reset( new reuse::TarFile(tarpath.c_str(), false,
				&reuse::gztar_ops_nonthread, true) );
		count = CountFiles(*tar, empty, &available, true);
		return available;
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::RestoreError(te.what());
	}
}

bool Restore::GetNextMeta(DBData &data)
{
	// always use m_record_data here, so that we don't lose access
	// to the actual record data for future calls to BuildRecord()
	// and FetchRecord()
	if( m_tar_record_state == RS_EMPTY ) {
		Retrieve(m_record_data);
	}

	// fill in the meta data that will be returned in the next call
	// to BuildRecord() or FetchRecord()... this is only valid if
	// the state is RS_NEXT
	switch( m_tar_record_state )
	{
	case RS_NEXT:
		data.SetVersion(Barry::DBData::REC_VERSION_1);
		data.SetDBName(m_current_dbname);
		data.SetIds(m_rec_type, m_unique_id);
		data.SetOffset(0);
		return true;

	default:
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Builder overrides

Restore::RetrievalState Restore::Retrieve(Data &record_data)
{
	// don't do anything unless we're empty
	if( m_tar_record_state != RS_EMPTY )
		return m_tar_record_state;

	// search for a valid record
	for(;;) {
		// load record data from tar file
		std::string filename;
		if( !m_tar->ReadNextFile(filename, record_data) ) {
			// assume end of file
			return m_tar_record_state = RS_EOF;
		}
		m_tar_record_state = RS_UNKNOWN;

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
			return m_tar_record_state = RS_NEXT;
		}

		// DIFFERENT DBNAME from here on down!
		m_tar_record_state = RS_DBEND;

		// does the filter allow this record?
		// if not, skip it and continue looking
		if( !IsSelected(dbname) ) {
			continue;
		}

		// all checks pass, load the new dbname, and return DBEND
		// if we are on a dbname boundary
		if( m_current_dbname.size() == 0 ) {
			// this is the first time through Retrieve, so ok
			m_tar_record_state = RS_NEXT;
		}

		m_current_dbname = dbname;
		return m_tar_record_state;
	}
}

bool Restore::BuildRecord(Barry::DBData &data,
			  size_t &offset,
			  const Barry::IConverter *ic)
{
	// in this case, we are loading into m_record_data anyway,
	// so no special handling is needed, like FetchRecord() needs.
	switch( Retrieve(m_record_data) )
	{
	case RS_NEXT:
		{
			data.SetVersion(Barry::DBData::REC_VERSION_1);
			data.SetDBName(m_current_dbname);
			data.SetIds(m_rec_type, m_unique_id);
			data.SetOffset(offset);

			int packet_size = offset + m_record_data.GetSize();
			unsigned char *buf = data.UseData().GetBuffer(packet_size);
			memcpy(buf + offset, m_record_data.GetData(), m_record_data.GetSize());
			offset += m_record_data.GetSize();
			data.UseData().ReleaseBuffer(packet_size);

			// clear loaded flag, as it has now been used
			m_tar_record_state = RS_EMPTY;
			return true;
		}

	case RS_EMPTY:
	case RS_UNKNOWN:
	default:
		throw std::logic_error("Invalid state in Restore::BuildRecord()");

	case RS_DBEND:
		// process the end of database by returning false
		// the next round will be valid, so set to RS_NEXT
		m_tar_record_state = RS_NEXT;
		return false;

	case RS_EOF:
		// always return false at end of file
		return false;
	}
}

bool Restore::FetchRecord(Barry::DBData &data, const Barry::IConverter *ic)
{
	// if the record has not yet been loaded, we can optimize
	// the buffer, and pass in our own... otherwise, just copy
	// the current buffer from m_record_data
	//
	// it is assumed here that Builder users will not alternate
	// between calls to BuildRecord() and FetchRecord()
	//
	if( m_tar_record_state == RS_EMPTY ) {
		// BUT, if RS_DBEND is the next value, then we need
		// to save the data for the next round... this
		// optimization is almost more bother than it's worth :-)
		if( Retrieve(data.UseData()) == RS_DBEND ) {
			m_record_data = data.GetData();
			m_tar_record_state = RS_NEXT;
			return false;
		}
	}
	else {
		data.UseData() = m_record_data;
	}

	switch( m_tar_record_state )
	{
	case RS_NEXT:
		data.SetVersion(Barry::DBData::REC_VERSION_1);
		data.SetDBName(m_current_dbname);
		data.SetIds(m_rec_type, m_unique_id);
		data.SetOffset(0);

		// clear loaded flag, as it has now been used
		m_tar_record_state = RS_EMPTY;
		return true;

	case RS_EMPTY:
	case RS_UNKNOWN:
	default:
		throw std::logic_error("Invalid state in Restore::FetchRecord()");

	case RS_DBEND:
		// process the end of database by returning false
		// the next round will be valid, so set to RS_NEXT
		m_tar_record_state = RS_NEXT;
		return false;

	case RS_EOF:
		// always return false at end of file
		return false;
	}
}

bool Restore::EndOfFile() const
{
	return m_tar_record_state == RS_EOF;
}

} // namespace Barry

