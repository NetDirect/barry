///
/// \file	r_memo.h
///		Record parsing class for the memo database.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)
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

#ifndef __BARRY_RECORD_MEMO_H__
#define __BARRY_RECORD_MEMO_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Memo
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	std::string Title;
	std::string Body;
	CategoryList Categories;

	UnknownsType Unknowns;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	Memo();
	~Memo();

	// Parser / Builder API (see parser.h / builder.h)
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

	bool operator<(const Memo &other) const;

	// database name
	static const char * GetDBName() { return "Memos"; }
	static uint8_t GetDefaultRecType() { return 0; }    // or 0?

	// Generic Field Handle support
	static const std::vector<FieldHandle<Memo> >& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Memo &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

