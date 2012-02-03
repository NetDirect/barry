///
/// \file	r_hhagent.h
///		Blackberry database record parser class for Handheld Agent records
///

/*
    Copyright (C) 2011-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_RECORD_HHAGENT_H__
#define __BARRY_RECORD_HHAGENT_H__

#include "dll.h"
#include "record.h"

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
// Handheld Agent record class
//
/// Represents a single record in the Handheld Agent database.
///
class BXEXPORT HandheldAgent
{
public:
	typedef Barry::UnknownsType			UnknownsType;

	//
	// Record fields
	//

	// contact specific data
	uint8_t RecType;
	uint32_t RecordId;

	std::string MEID;
	std::string Model;
	std::string Bands;
	std::string Pin;		// may not be valid for every record
	std::string Version;
	std::string PlatformVersion;
	std::string Manufacturer;
	std::string Network;

	UnknownsType Unknowns;

public:
	const unsigned char* ParseField(const unsigned char *begin,
		const unsigned char *end, const IConverter *ic = 0);

public:
	HandheldAgent();
	~HandheldAgent();

	uint32_t GetID() const { return RecordId; }
	std::string GetFullName() const;
	const std::string& GetEmail(unsigned int index = 0) const;

	// Parser / Builder API (see parser.h / builder.h)
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset, const IConverter *ic = 0);
	void BuildHeader(Data &data, size_t &offset) const;
	void BuildFields(Data &data, size_t &offset, const IConverter *ic = 0) const;

	// operations (common among record classes)
	void Clear();			// erase everything
	void Dump(std::ostream &os) const;
	std::string GetDescription() const;

	// Sorting - use enough data to make the sorting as
	//           consistent as possible
	bool operator<(const HandheldAgent &other) const;

	// database name
	static const char * GetDBName() { return "Handheld Agent"; }
	static uint8_t GetDefaultRecType() { return 0; }
	static uint32_t GetMEIDRecordId() { return 0x3000000; }
	static uint32_t GetUnknown1RecordId() { return 0x4000000; }
	static uint32_t GetUnknown2RecordId() { return 0x5000000; }
	static uint32_t GetUnknown3RecordId() { return 0x7000000; }
	static bool IsSpecial(uint32_t record_id);

	// utility functions
	static bool IsESNHex(const std::string &esn);
	static bool IsESNDec(const std::string &esn);
	static std::string ESNDec2Hex(const std::string &esn);
	static std::string ESNHex2Dec(const std::string &esn);

	// Generic Field Handle support
	static const FieldHandle<HandheldAgent>::ListT& GetFieldHandles();
};

BXEXPORT inline std::ostream& operator<< (std::ostream &os, const HandheldAgent &hha) {
	hha.Dump(os);
	return os;
}

/// @}

} // namespace Barry

#endif

