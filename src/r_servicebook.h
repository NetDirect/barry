///
/// \file	r_servicebook.h
///		Blackberry database record parser class for the
///		Service Book record.
///

/*
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_SERVICEBOOK_H__
#define __BARRY_RECORD_SERVICEBOOK_H__

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

// This is a packed field, which is a group of fields packed in
// variable length records inside one larger field of a normal record.
class BXEXPORT ServiceBookConfig
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	uint8_t Format;

	UnknownsType Unknowns;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	ServiceBookConfig();
	~ServiceBookConfig();

	// Parser / Builder API (see parser.h / builder.h)
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	void Clear();

	void Dump(std::ostream &os) const;
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const ServiceBookConfig &msg) {
	msg.Dump(os);
	return os;
}


class ServiceBookData;

class BXEXPORT ServiceBook
{
	ServiceBookData *m_data;

public:
	typedef std::vector<UnknownField>		UnknownsType;

	uint8_t RecType;
	uint32_t RecordId;
	std::string Name;
	std::string HiddenName;
	std::string Description;
	std::string DSID;
	std::string BesDomain;
	std::string UniqueId;
	std::string ContentId;
	ServiceBookConfig Config;

	UnknownsType Unknowns;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	ServiceBook();
	~ServiceBook();

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// sorting
	bool operator<(const ServiceBook &other) const;

	// database name
	static const char * GetDBName() { return "Service Book"; }
	static uint8_t GetDefaultRecType() { return 0; }
};

BXEXPORT inline std::ostream& operator<<(std::ostream &os, const ServiceBook &msg) {
	msg.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

