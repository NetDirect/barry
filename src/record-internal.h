///
/// \file	record-internal.h
///		Support functions, types, and templates for the
///		general record parsing classes in r_*.h files.
///		This header is NOT installed for applications to
///		use, so it is safe to put library-specific things
///		in here.
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

#ifndef __BARRY_RECORD_INTERNAL_H__
#define __BARRY_RECORD_INTERNAL_H__

#include <string>
#include "protostructs.h"
#include "error.h"
#include "endian.h"
#include "record.h"

// forward declarations
namespace Barry {
	class Data;
	class IConverter;
}

namespace Barry {

template <class RecordT>
const unsigned char*  ParseCommonFields(RecordT &rec,
					const void *begin,
					const void *end,
					const IConverter *ic = 0)
{
	const unsigned char *b = (const unsigned char*) begin;
	const unsigned char *e = (const unsigned char*) end;

	while( (b + COMMON_FIELD_HEADER_SIZE) < e )
		b = rec.ParseField(b, e, ic);
	return b;
}

// Use templates here to guarantee types are converted in the strictest manner.
template <class SizeT>
inline SizeT ConvertHtoB(SizeT s)
{
	throw Error("Not implemented.");
}

// specializations for specific sizes
template <> inline uint8_t ConvertHtoB<uint8_t>(uint8_t s)    { return s; }
template <> inline uint16_t ConvertHtoB<uint16_t>(uint16_t s) { return htobs(s); }
template <> inline uint32_t ConvertHtoB<uint32_t>(uint32_t s) { return htobl(s); }
template <> inline uint64_t ConvertHtoB<uint64_t>(uint64_t s) { return htobll(s); }


template <class RecordT>
struct FieldLink
{
	int type;
	const char *name;
	const char *ldif;
	const char *objectClass;
	std::string RecordT::* strMember;	// FIXME - find a more general
	EmailAddressList RecordT::* addrMember;	// way to do this...
	time_t RecordT::* timeMember;
	PostalAddress RecordT::* postMember;
	std::string PostalAddress::* postField;
	bool iconvNeeded;
};

void BuildField1900(Data &data, size_t &size, uint8_t type, time_t t);
void BuildField(Data &data, size_t &size, uint8_t type, char c);
void BuildField(Data &data, size_t &size, uint8_t type, uint8_t c);
void BuildField(Data &data, size_t &size, uint8_t type, uint16_t value);
void BuildField(Data &data, size_t &size, uint8_t type, uint32_t value);
void BuildField(Data &data, size_t &size, uint8_t type, uint64_t value);
void BuildField(Data &data, size_t &size, uint8_t type, const std::string &str);
void BuildField(Data &data, size_t &size, uint8_t type, const void *buf, size_t bufsize);
void BuildField(Data &data, size_t &size, const Barry::UnknownField &field);
void BuildField(Data &data, size_t &size, uint8_t type, const Barry::Protocol::GroupLink &link);
std::string ParseFieldString(const Barry::Protocol::CommonField *field);
std::string ParseFieldString(const void *data, uint16_t maxlen);


// Functions to help build JDWP command packets
void AddJDWByte(Barry::Data &data, size_t &size, const uint8_t value);
void AddJDWInt(Barry::Data &data, size_t &size, const uint32_t value);
void AddJDWChar(Barry::Data &data, size_t &size, const void *buf, size_t bufsize);
void AddJDWString(Barry::Data &data, size_t &size, const std::string &str);

} // namespace Barry

#endif

