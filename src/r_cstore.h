///
/// \file	r_cstore.h
///		Blackberry database record parser class for
///		Content Store records.
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

#ifndef __BARRY_RECORD_CSTORE_H__
#define __BARRY_RECORD_CSTORE_H__

#include "dll.h"
#include "record.h"
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

//
// NOTE:  All classes here must be container-safe!  Perhaps add sorting
//        operators in the future.
//

/// \addtogroup RecordParserClasses
/// @{

//
// Content Store record class
//
/// Represents a single record in the Content Store Blackberry database.
///
class BXEXPORT ContentStore
{
public:
	typedef Barry::UnknownsType		UnknownsType;

	//
	// Record fields
	//

	// contact specific data
	uint8_t RecType;
	uint32_t RecordId;

	std::string Filename;
	bool FolderFlag;
	std::string FileContent;
	std::string FileDescriptor;

	UnknownsType Unknowns;

private:
	uint64_t FileSize;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	ContentStore();
	~ContentStore();

	// operations (common among record classes)
	void Clear();			// erase everything
	void Dump(std::ostream &os) const;
	std::string GetDescription() const;

	// Sorting - use enough data to make the sorting as
	//           consistent as possible
	bool operator<(const ContentStore &other) const;

	// Parser / Builder API (see parser.h / builder.h)
	void Validate() const;
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;
	static const char * GetDBName() { return "Content Store"; }
	static uint8_t GetDefaultRecType() { return 0; }

	// Generic Field Handle support
	static const FieldHandle<ContentStore>::ListT& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os,
					const ContentStore &cstore)
{
	cstore.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

