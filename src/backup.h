///
/// \file	backup.h
///		Special parser class to support creation of Barry Backup files
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

#ifndef __BARRYBACKUP_BACKUP_H__
#define __BARRYBACKUP_BACKUP_H__

#include "dll.h"
#include "parser.h"
#include <string>
#include <memory>

// forward declarations
namespace reuse {
	class TarFile;
}

namespace Barry {

class BXEXPORT Backup : public Barry::Parser
{
private:
	std::auto_ptr<reuse::TarFile> m_tar;

	std::string m_current_dbname;
	std::string m_tar_id_text;
	std::string m_record_data;

public:
	explicit Backup(const std::string &tarpath);
	~Backup();

	void Close();

	// Barry::Parser overrides
	virtual void Clear();
	virtual void SetIds(const std::string &DbName,
			uint8_t RecType, uint32_t UniqueId);
	virtual void ParseHeader(const Barry::Data &data, size_t &offset);
	virtual void ParseFields(const Barry::Data &data, size_t &offset,
			const Barry::IConverter *ic);
	virtual void Store();
};

} // namespace Barry

#endif

