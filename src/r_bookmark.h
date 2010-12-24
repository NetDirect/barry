///
/// \file	r_bookmark.h
///		Record parsing class for call logs
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_BOOKMARK_H__
#define __BARRY_RECORD_BOOKMARK_H__

#include "dll.h"
#include "record.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace Barry {

// forward declarations
class IConverter;

class BXEXPORT Bookmark
{
public:
	typedef Barry::UnknownsType		UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;

	uint8_t Index;

	std::string Name;
	std::string Icon;
	std::string Url;

	enum BrowserIdentityType
	{
		IdentityAuto = 0,
		IdentityBlackBerry,
		IdentityFireFox,
		IdentityInternetExplorer,
		IdentityUnknown
	};
	BrowserIdentityType BrowserIdentity;

	enum DisplayModeType
	{
		DisplayAuto = 0,
		DisplayColomn,
		DisplayPage,
		DisplayUnknown
	};
	DisplayModeType DisplayMode;

	enum JavaScriptModeType
	{
		JavaScriptAuto = 0,
		JavaScriptEnabled,
		JavaScriptDisabled,
		JavaScriptUnknown
	};
	JavaScriptModeType JavaScriptMode;

	UnknownsType Unknowns;

protected:
	const unsigned char* ParseStruct1Field(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	const unsigned char* ParseStruct2(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	Bookmark();
	~Bookmark();

	// Parser / Builder API (see parser.h / builder.h)
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);

	void Clear();

	void Dump(std::ostream &os) const;

	// Sorting - use enough data to make the sorting as
	//           consistent as possible
	bool operator<(const Bookmark &other) const;

	// database name
	static const char * GetDBName() { return "Browser Bookmarks"; }
	static uint8_t GetDefaultRecType() { return 1; }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const Bookmark &msg) {
	msg.Dump(os);
	return os;
}

} // namespace Barry

#endif

