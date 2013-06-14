///
/// \file	r_folder.h
///		Record parsing class for the Folder database.
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)
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
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	std::string Name;
	uint16_t Number;	// Not unique, used for ordering of subfolders - NOT level
	uint16_t Level;	// From parent

	enum FolderType {
		FolderSubtree = 0,
		FolderDeleted,
		FolderInbox,
		FolderOutbox,
		FolderSent,
		FolderOther,
		FolderDraft = 0x0a
	};
	FolderType Type;

	enum FolderStatusType {
		FolderOrphan = 0x50,
		FolderUnfiled,
		FolderFiled
	};

	UnknownsType Unknowns;

protected:
	static FolderType TypeProto2Rec(uint8_t t);
	static uint8_t TypeRec2Proto(FolderType t);

public:
	Folder();
	~Folder();

	// Parser / Builder API (see parser.h / builder.h)
	void Validate() const;
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	// operations (common among record classes)
	void Clear();
	void Dump(std::ostream &os) const;
	std::string GetDescription() const;

	bool operator<(const Folder &other) const { return Name < other.Name; }

	// database name
	static const char * GetDBName() { return "Folders"; }
	static uint8_t GetDefaultRecType() { return 0; }

	// Generic Field Handle support
	static const FieldHandle<Folder>::ListT& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Folder &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif	// __BARRY_RECORD_FOLDER_H__


