///
/// \file	backup.cc
///		Special parser class to support creation of Barry Backup files
///

/*
    Copyright (C) 2010-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "backup.h"
#include "tarfile.h"
#include "error.h"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace Barry {

Backup::Backup(const std::string &tarpath)
{
	try {
		m_tar.reset( new reuse::TarFile(tarpath.c_str(), true,
				&reuse::gztar_ops_nonthread, true) );
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::BackupError(te.what());
	}
}

Backup::~Backup()
{
	try {
		Close();
	}
	catch( Barry::BackupError & ) {
		// throw it away
	}
}

void Backup::Close()
{
	if( m_tar.get() ) try {
		m_tar->Close();
		m_tar.reset();
	}
	catch( reuse::TarFile::TarError &te ) {
		throw Barry::BackupError(te.what());
	}
}

void Backup::ClearStats()
{
	m_stats.clear();
}


//////////////////////////////////////////////////////////////////////////////
// Barry::Parser overrides

void Backup::ParseRecord(const Barry::DBData &data,
			  const Barry::IConverter *ic)
{
	m_current_dbname = data.GetDBName();

	std::ostringstream oss;
	oss << std::hex << data.GetUniqueId()
		<< " " << (unsigned int)data.GetRecType();
	m_tar_id_text = oss.str();

	if( m_current_dbname.size() == 0 )
		throw Barry::BackupError(_("Backup: No database name available"));
	if( m_tar_id_text.size() == 0 )
		throw Barry::BackupError(_("Backup: No unique ID available!"));

	m_record_data.assign(
		(const char*)data.GetData().GetData() + data.GetOffset(),
		data.GetData().GetSize() - data.GetOffset());

	// save to tarball
	std::string tarname = m_current_dbname + "/" + m_tar_id_text;
	m_tar->AppendFile(tarname.c_str(), m_record_data);

	// add stats
	m_stats[m_current_dbname]++;
}

} // namespace Barry

