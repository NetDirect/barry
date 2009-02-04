///
/// \file	r_folder.h
///		Record parsing class for the Folder database.
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2007, Brian Edginton

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

#ifndef __BARRY_RECORD_FOLDER_H__
#define __BARRY_RECORD_FOLDER_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Folder
{
public:
	typedef std::vector<UnknownField>			UnknownsType;
	uint8_t RecType;
	uint32_t RecordId;

	std::string FolderName;
	uint16_t	FolderNumber;	// Not unique, used for ordering of subfolders - NOT level
	uint16_t	FolderLevel;	// From parent

	enum FolderTypeEnum {
		FolderSubtree = 0,
		FolderDeleted,
		FolderInbox,
		FolderOutbox,
		FolderSent,
		FolderOther,
		FolderDraft = 0x0a
	};
	FolderTypeEnum FolderType;

	enum FolderStatusType {
		FolderOrphan = 0x50,
		FolderUnfiled,
		FolderFiled
	};

	UnknownsType Unknowns;

public:
	Folder();
	~Folder();

	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;
	bool operator<(const Folder &other) const { return FolderName < other.FolderName; }

	// database name
	static const char * GetDBName() { return "Folders"; }
	static uint8_t GetDefaultRecType() { return 0; }

};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Folder &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif	// __BARRY_RECORD_FOLDER_H__


