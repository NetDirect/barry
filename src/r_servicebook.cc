///
/// \file	r_servicebook.cc
///		Blackberry database record parser class for
///		Service Book records.
///

/*
    Copyright (C) 2005-2008, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "r_servicebook.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include <ostream>
#include <iomanip>
#include <time.h>
#include <stdexcept>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {


///////////////////////////////////////////////////////////////////////////////
// ServiceBookConfig class

// service book packed field codes
#define SBFCC_END			0xffff

static FieldLink<ServiceBookConfig> ServiceBookConfigFieldLinks[] = {
//   { SBFC_DSID,        "DSID",       0, 0,    &ServiceBook::DSID, 0, 0 },
   { SBFCC_END,         "End of List",0, 0,    0, 0, 0 }
};

ServiceBookConfig::ServiceBookConfig()
	: Format(0)
{
	Clear();
}

ServiceBookConfig::~ServiceBookConfig()
{
}

const unsigned char* ServiceBookConfig::ParseField(const unsigned char *begin,
						   const unsigned char *end,
						   const IConverter *ic)
{
	const void *raw;
	uint16_t size, type;

	switch( Format )
	{
	case 0x02:
		{
			const PackedField_02 *field = (const PackedField_02 *) begin;
			raw = field->raw;
			size = field->size;
			type = field->type;
			begin += PACKED_FIELD_02_HEADER_SIZE + size;
		}
		break;

	case 0x10:
		{
			const PackedField_10 *field = (const PackedField_10 *) begin;
			raw = field->raw;
			size = field->size;
			type = field->type;
			begin += PACKED_FIELD_10_HEADER_SIZE + size;
		}
		break;

	default:
		eout("Unknown packed field format" << Format);
		return begin + 1;
	}


	// check size
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<ServiceBookConfig> *b = ServiceBookConfigFieldLinks;
		b->type != SBFCC_END;
		b++ )
	{
		if( b->type == type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(raw, size-1);
				return begin;	// done!
			}
		}
	}

/*
	// handle special cases
	switch( type )
	{
	}
*/

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = type;
	uf.data.assign((const char*)raw, size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void ServiceBookConfig::ParseHeader(const Data &data, size_t &offset)
{
	MAKE_RECORD(const Barry::Protocol::ServiceBookConfigField, sbc, data, offset);
	offset += SERVICE_BOOK_CONFIG_FIELD_HEADER_SIZE;
	if( data.GetSize() >= offset ) {	// size check!
		Format = sbc->format;
	}
}

void ServiceBookConfig::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void ServiceBookConfig::BuildHeader(Data &data, size_t &offset) const
{
	// make sure there is enough space
	data.GetBuffer(offset + SERVICE_BOOK_CONFIG_FIELD_HEADER_SIZE);

	MAKE_RECORD(Barry::Protocol::ServiceBookConfigField, sbc, data, offset);
	sbc->format = Format;

	offset += SERVICE_BOOK_CONFIG_FIELD_HEADER_SIZE;
}

//
// BuildFields
//
/// Build fields part of record
///
void ServiceBookConfig::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	throw std::logic_error("ServiceBookConfig::Build not yet implemented");
}

void ServiceBookConfig::Clear()
{
	Unknowns.clear();
}

void ServiceBookConfig::Dump(std::ostream &os) const
{
	os << "   ServiceBookConfig Format: " << setbase(16) << (uint16_t)Format << "\n";

	// cycle through the type table
	for(	const FieldLink<ServiceBookConfig> *b = ServiceBookConfigFieldLinks;
		b->type != SBFCC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "      " << b->name << ": " << s << "\n";
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				os << "      " << b->name << ": " << ctime(&t);
		}
	}

	// print any unknowns
	os << Unknowns;
	os << "   ------------------- End of Config Field\n";
}


///////////////////////////////////////////////////////////////////////////////
// ServiceBook class

// service book field codes
#define SBFC_OLD_NAME			0x01
#define SBFC_HIDDEN_NAME		0x02
#define SBFC_NAME			0x03
#define SBFC_OLD_UNIQUE_ID		0x06
#define SBFC_UNIQUE_ID			0x07
#define SBFC_CONTENT_ID			0x08
#define SBFC_CONFIG			0x09
#define SBFC_OLD_DESC			0x32
#define SBFC_DESCRIPTION		0x0f
#define SBFC_DSID			0xa1
#define SBFC_BES_DOMAIN			0xa2
#define SBFC_USER_ID			0xa3
#define SBFC_END			0xffff

static FieldLink<ServiceBook> ServiceBookFieldLinks[] = {
   { SBFC_HIDDEN_NAME, "Hidden Name",0, 0,    &ServiceBook::HiddenName, 0, 0 },
   { SBFC_DSID,        "DSID",       0, 0,    &ServiceBook::DSID, 0, 0 },
   { SBFC_END,         "End of List",0, 0,    0, 0, 0 }
};

ServiceBook::ServiceBook()
	: NameType(SBFC_OLD_NAME),
	DescType(SBFC_OLD_DESC),
	UniqueIdType(SBFC_OLD_UNIQUE_ID),
	RecordId(0)
{
	Clear();
}

ServiceBook::~ServiceBook()
{
}

const unsigned char* ServiceBook::ParseField(const unsigned char *begin,
					     const unsigned char *end,
					     const IConverter *ic)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )	// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<ServiceBook> *b = ServiceBookFieldLinks;
		b->type != SBFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				return begin;	// done!
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				time_t &t = this->*(b->timeMember);
				t = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case SBFC_OLD_NAME:		// strings with old/new type codes
	case SBFC_NAME:
		Name = ParseFieldString(field);
		NameType = field->type;
		return begin;

	case SBFC_OLD_DESC:
	case SBFC_DESCRIPTION:
		Description = ParseFieldString(field);
		DescType = field->type;
		return begin;

	case SBFC_OLD_UNIQUE_ID:
	case SBFC_UNIQUE_ID:
		UniqueId = ParseFieldString(field);
		UniqueIdType = field->type;
		return begin;

	case SBFC_CONTENT_ID:
		ContentId = ParseFieldString(field);
		return begin;

	case SBFC_BES_DOMAIN:
		BesDomain = ParseFieldString(field);
		return begin;

	case SBFC_CONFIG:
		{
			Data config((const void *)field->u.raw, btohs(field->size));
			size_t offset = 0;
			Config.ParseHeader(config, offset);
			Config.ParseFields(config, offset);
		}
		break;	// break here so raw packet is still visible in dump
//		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void ServiceBook::ParseHeader(const Data &data, size_t &offset)
{
	// no header in this record (?)
}

void ServiceBook::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void ServiceBook::BuildHeader(Data &data, size_t &offset) const
{
	// no header in this record (?)
}

//
// BuildFields
//
/// Build fields part of record
///
void ServiceBook::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	throw std::logic_error("ServiceBook::BuildFields not yet implemented");
}

void ServiceBook::Clear()
{
	Unknowns.clear();
	Config.Clear();
}

void ServiceBook::Dump(std::ostream &os) const
{
	os << "ServiceBook entry: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";

	// cycle through the type table
	for(	const FieldLink<ServiceBook> *b = ServiceBookFieldLinks;
		b->type != SBFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				os << "   " << b->name << ": " << s << "\n";
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				os << "   " << b->name << ": " << ctime(&t);
		}
	}

	// special cases
	if( UniqueId.size() )
		os << "   Unique ID: " << UniqueId << "\n";
	if( ContentId.size() )
		os << "   Content ID: " << ContentId << "\n";
	if( BesDomain.size() )
		os << "   (BES) Domain: " << BesDomain << "\n";

	os << Config;

	// print any unknowns
	os << Unknowns;
}

} // namespace Barry

