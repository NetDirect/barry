///
/// \file	r_servicebook.cc
///		Blackberry database record parser class for
///		Service Book records.
///

/*
    Copyright (C) 2005-2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "iconv.h"
#include <ostream>
#include <iomanip>
#include <time.h>
#include <stdexcept>
#include "ios_state.h"

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
	case 0x01:
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
		eout("------> Unknown packed field format: 0x" << std::hex <<
			(unsigned int) Format);
		throw BadPackedFormat(Format);
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
	Format = 0;
	Unknowns.clear();
}

void ServiceBookConfig::Dump(std::ostream &os) const
{
	ios_format_state state(os);

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

// private data class, containing internal structures
class ServiceBookData
{
public:
	FieldLink<ServiceBook> *m_typeSet;
	ServiceBookData(FieldLink<ServiceBook> *typeSet) : m_typeSet(typeSet) {}
};

// The Old/New tables contain the same fields, but use different
// type codes.  Keeping them separate yet linked makes it possible
// to convert between old and new type codes, while hopefully keeping
// things generic.
static FieldLink<ServiceBook> ServiceBookOldFieldLinks[] = {
   { SBFC_OLD_NAME,      "Old Name", 0, 0,     &ServiceBook::Name, 0, 0, 0, 0, true },
   { SBFC_OLD_DESC,      "Old Desc", 0, 0,     &ServiceBook::Description, 0, 0, 0, 0, true },
   { SBFC_OLD_UNIQUE_ID, "Old UniqueId", 0, 0, &ServiceBook::UniqueId, 0, 0, 0, 0, false },
   { SBFC_END,           "End of List", 0, 0,  0, 0, 0, 0, 0, false }
};

static FieldLink<ServiceBook> ServiceBookNewFieldLinks[] = {
   { SBFC_NAME,        "Name", 0, 0,         &ServiceBook::Name, 0, 0, 0, 0, true },
   { SBFC_DESCRIPTION, "Description", 0, 0,  &ServiceBook::Description, 0, 0, 0, 0, true },
   { SBFC_UNIQUE_ID,   "UniqueId", 0, 0,     &ServiceBook::UniqueId, 0, 0, 0, 0, false },
   { SBFC_END,         "End of List", 0, 0,  0, 0, 0, 0, 0, false }
};

// This table holds all
static FieldLink<ServiceBook> ServiceBookFieldLinks[] = {
   { SBFC_HIDDEN_NAME, "Hidden Name",0, 0, &ServiceBook::HiddenName, 0, 0, 0, 0, true },
   { SBFC_DSID,        "DSID",       0, 0, &ServiceBook::DSID, 0, 0, 0, 0, false },
   { SBFC_CONTENT_ID,  "ContentId",  0, 0, &ServiceBook::ContentId, 0, 0, 0, 0, false },
   { SBFC_BES_DOMAIN,  "BES Domain", 0, 0, &ServiceBook::BesDomain, 0, 0, 0, 0, false },
   { SBFC_END,         "End of List",0, 0, 0, 0, 0, 0, 0, false }
};

// Array of conflicting tables only
static FieldLink<ServiceBook> *ServiceBookLinkTable[] = {
   ServiceBookOldFieldLinks,
   ServiceBookNewFieldLinks,
   0
};

#define FIELDLINK_END 0xffff

template <class RecordT>
FieldLink<RecordT>* ParseFieldByTable(RecordT *rec,
				      const CommonField *field,
				      const IConverter *ic,
				      FieldLink<RecordT> *links)
{
	// cycle through the type table
	for( FieldLink<RecordT> *b = links; b->type != FIELDLINK_END; b++ ) {
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = rec->*(b->strMember);
				if( s.size() ) {
					dout(RecordT::GetDBName() << ": field '" << b->name << "' already has data (" << s << "). Overwriting.");
				}
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return links;
			}
			else if( b->timeMember && btohs(field->size) == 4 ) {
				time_t &t = rec->*(b->timeMember);
				t = min2time(field->u.min1900);
				return links;
			}
		}
	}
	return 0;
}

template <class RecordT>
FieldLink<RecordT>* ParseFieldByTable(RecordT *rec,
				      const CommonField *field,
				      const IConverter *ic,
				      FieldLink<RecordT> **b)
{
	for( ; *b; b++ ) {
		FieldLink<RecordT> *link =
			ParseFieldByTable<RecordT>(rec, field, ic, *b);
		if( link )
			return link;
	}
	return 0;
}

ServiceBook::ServiceBook()
	: m_data( new ServiceBookData(ServiceBookOldFieldLinks) )
	, RecordId(0)
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

	// cycle through the type tables
	FieldLink<ServiceBook> *typeSet =
		ParseFieldByTable(this, field, ic, ServiceBookLinkTable);
	if( typeSet ) {
		if( m_data->m_typeSet && m_data->m_typeSet != typeSet ) {
			dout("ServiceBook record has a mix of old and new field types.");
		}
		m_data->m_typeSet = typeSet;
		return begin;
	}
	else {
		if( ParseFieldByTable(this, field, ic, ServiceBookFieldLinks) )
			return begin;	// done!
	}

	// handle special cases
	switch( field->type )
	{
	case SBFC_CONFIG:
		try {
			Data config((const void *)field->u.raw, btohs(field->size));
			size_t offset = 0;
			Config.ParseHeader(config, offset);
			Config.ParseFields(config, offset);
			return begin;	// success
		}
		catch( BadPackedFormat & ) {
			// break here so unprocessed raw packet is still
			// visible in dump
			break;
		}
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
	m_data->m_typeSet = ServiceBookOldFieldLinks;
	Unknowns.clear();
	Config.Clear();
}

std::string ServiceBook::GetDescription() const
{
	return Name;
}

inline void FormatStr(std::ostream &os, const char *name, const std::string &str)
{
	ios_format_state state(os);

	if( str.size() ) {
		os << "   " << setw(20) << name;
		os << ": " << str << "\n";
	}
}

void ServiceBook::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os.setf(ios::left);
	os.fill(' ');

	os << "ServiceBook entry: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";

	FormatStr(os, "Name", Name);
	FormatStr(os, "Hidden Name", HiddenName);
	FormatStr(os, "Description", Description);
	FormatStr(os, "DSID", DSID);
	FormatStr(os, "Unique ID", UniqueId);
	FormatStr(os, "Content ID", ContentId);
	FormatStr(os, "(BES) Domain", BesDomain);

	os << Config;

	// print any unknowns
	os << Unknowns;
}

bool ServiceBook::operator<(const ServiceBook &other) const
{
	int cmp = BesDomain.compare(other.BesDomain);
	if( cmp == 0 )
		cmp = DSID.compare(other.DSID);
	if( cmp == 0 )
		cmp = Name.compare(other.Name);
	if( cmp == 0 )
		cmp = UniqueId.compare(other.UniqueId);
	return cmp < 0;
}

} // namespace Barry

