///
/// \file	record.cc
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///		This header provides the common types and classes
///		used by the general record parser classes in the
///		r_*.h files.  Only application-safe API stuff goes in
///		here.  Internal library types go in record-internal.h
///

/*
    Copyright (C) 2005-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "record.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include <sstream>
#include <iomanip>
#include <time.h>
#include <string.h>
#include <stdio.h>			// for sscanf()
#include <stdexcept>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

//////////////////////////////////////////////////////////////////////////////
// Field builder helper functions

void BuildField1900(Data &data, size_t &size, uint8_t type, time_t t)
{
	size_t timesize = COMMON_FIELD_MIN1900_SIZE;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + timesize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(timesize);
	field->type = type;
	field->u.min1900 = time2min(t);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, char c)
{
	BuildField(data, size, type, (uint8_t)c);
}

void BuildField(Data &data, size_t &size, uint8_t type, uint8_t c)
{
	size_t strsize = 1;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;
	memcpy(field->u.raw, &c, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, uint16_t value)
{
	size_t strsize = 2;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;

	uint16_t store = htobs(value);
	memcpy(field->u.raw, &store, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, uint32_t value)
{
	size_t strsize = 4;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobl(strsize);
	field->type = type;

	uint32_t store = htobs(value);
	memcpy(field->u.raw, &store, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, const std::string &str)
{
	// include null terminator
	BuildField(data, size, type, str.c_str(), str.size() + 1);
}

void BuildField(Data &data, size_t &size, uint8_t type,
		const void *buf, size_t bufsize)
{
	// include null terminator
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + bufsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(bufsize);
	field->type = type;
	memcpy(field->u.raw, buf, bufsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, const Barry::UnknownField &field)
{
	BuildField(data, size, field.type,
		field.data.raw_data.data(), field.data.raw_data.size());
}

void BuildField(Data &data, size_t &size, uint8_t type, const Barry::Protocol::GroupLink &link)
{
	size_t linksize = sizeof(Barry::Protocol::GroupLink);
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + linksize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(linksize);
	field->type = type;
	field->u.link = link;

	size += fieldsize;
}

std::string ParseFieldString(const Barry::Protocol::CommonField *field)
{
	// make no assumptions here, and pass the full size in as
	// the maxlen, even though 99% of the time, it will be a null...
	// this function can be used by non-null terminated strings as well
	return ParseFieldString(field->u.raw, btohs(field->size));
}

std::string ParseFieldString(const void *data, uint16_t maxlen)
{
	const char *str = (const char *)data;

	// find last non-null character, since some fields
	// can have multiple null terminators
	while( maxlen && str[maxlen-1] == 0 )
		maxlen--;

	return std::string(str, maxlen);
}


///////////////////////////////////////////////////////////////////////////////
// CommandTable class

CommandTable::CommandTable()
{
}

CommandTable::~CommandTable()
{
}

const unsigned char* CommandTable::ParseField(const unsigned char *begin,
					      const unsigned char *end)
{
	// check if there is enough data for a header
	const unsigned char *headend = begin + sizeof(CommandTableField);
	if( headend > end )
		return headend;

	const CommandTableField *field = (const CommandTableField *) begin;

	// advance and check size
	begin += COMMAND_FIELD_HEADER_SIZE + field->size;	// size is byte
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	Command command;
	command.Code = field->code;
	command.Name.assign((const char *)field->name, field->size);
	Commands.push_back(command);
	return begin;
}

void CommandTable::Parse(const Data &data, size_t offset)
{
	if( offset >= data.GetSize() )
		return;

	const unsigned char *begin = data.GetData() + offset;
	const unsigned char *end = data.GetData() + data.GetSize();

	while( begin < end )
		begin = ParseField(begin, end);
}

void CommandTable::Clear()
{
	Commands.clear();
}

unsigned int CommandTable::GetCommand(const std::string &name) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	for( ; b != Commands.end(); b++ )
		if( b->Name == name )
			return b->Code;
	return 0;
}

void CommandTable::Dump(std::ostream &os) const
{
	CommandArrayType::const_iterator b = Commands.begin();
	os << "Command table:\n";
	for( ; b != Commands.end(); b++ ) {
		os << "    Command: 0x" << setbase(16) << b->Code
		   << " '" << b->Name << "'\n";
	}
}



///////////////////////////////////////////////////////////////////////////////
// RecordStateTable class

RecordStateTable::RecordStateTable()
	: m_LastNewRecordId(1)
{
}

RecordStateTable::~RecordStateTable()
{
}

const unsigned char* RecordStateTable::ParseField(const unsigned char *begin,
						  const unsigned char *end)
{
	const RecordStateTableField *field = (const RecordStateTableField *) begin;

	// advance and check size
	begin += sizeof(RecordStateTableField);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	State state;
	state.Index = btohs(field->index);
	state.RecordId = btohl(field->uniqueId);
	state.Dirty = (field->flags & BARRY_RSTF_DIRTY) != 0;
	state.RecType = field->rectype;
	state.Unknown2.assign((const char*)field->unknown2, sizeof(field->unknown2));
	StateMap[state.Index] = state;

	return begin;
}

void RecordStateTable::Parse(const Data &data)
{
	size_t offset = 12;	// skipping the unknown 2 bytes at start

	if( offset >= data.GetSize() )
		return;

	const unsigned char *begin = data.GetData() + offset;
	const unsigned char *end = data.GetData() + data.GetSize();

	while( begin < end )
		begin = ParseField(begin, end);
}

void RecordStateTable::Clear()
{
	StateMap.clear();
	m_LastNewRecordId = 1;
}

// Searches the StateMap table for RecordId, and returns the "index"
// in the map if found.  Returns true if found, false if not.
// pFoundIndex can be null if only the existence of the index is desired
bool RecordStateTable::GetIndex(uint32_t RecordId, IndexType *pFoundIndex) const
{
	StateMapType::const_iterator i = StateMap.begin();
	for( ; i != StateMap.end(); ++i ) {
		if( i->second.RecordId == RecordId ) {
			if( pFoundIndex )
				*pFoundIndex = i->first;
			return true;
		}
	}
	return false;
}

// Generate a new RecordId that is not in the state table.
// Starts at 1 and keeps incrementing until a free one is found.
uint32_t RecordStateTable::MakeNewRecordId() const
{
	// start with next Id
	m_LastNewRecordId++;

	// make sure it doesn't already exist
	StateMapType::const_iterator i = StateMap.begin();
	while( i != StateMap.end() ) {
		if( m_LastNewRecordId == i->second.RecordId ) {
			m_LastNewRecordId++;		// try again
			i = StateMap.begin();		// start over
		}
		else {
			++i;				// next State
		}
	}
	return m_LastNewRecordId;
}

void RecordStateTable::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::right);
	char fill = os.fill(' ');
	bool bPrintAscii = Data::PrintAscii();
	Data::PrintAscii(false);

	os << "  Index  RecordId    Dirty  RecType" << endl;
	os << "-------  ----------  -----  -------" << endl;

	StateMapType::const_iterator b, e = StateMap.end();
	for( b = StateMap.begin(); b != e ; ++b ) {
		const State &state = b->second;

		os.fill(' ');
		os << setbase(10) << setw(7) << state.Index;
		os << "  0x" << setbase(16) << setfill('0') << setw(8) << state.RecordId;
		os << "  " << setfill(' ') << setw(5) << (state.Dirty ? "yes" : "no");
		os << "     0x" << setbase(16) << setfill('0') << setw(2) << state.RecType;
		os << "   " << Data(state.Unknown2.data(), state.Unknown2.size());
	}

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
	Data::PrintAscii(bPrintAscii);
}



///////////////////////////////////////////////////////////////////////////////
// DatabaseDatabase class

DatabaseDatabase::DatabaseDatabase()
{
}

DatabaseDatabase::~DatabaseDatabase()
{
}

template <class RecordType, class FieldType>
void DatabaseDatabase::ParseRec(const RecordType &rec, const unsigned char *end)
{
}

template <class FieldType>
const unsigned char* DatabaseDatabase::ParseField(const unsigned char *begin,
						  const unsigned char *end)
{
	// check if there is enough data for a header
	const unsigned char *headend = begin + sizeof(FieldType);
	if( headend > end )
		return headend;

	// get our header
	const FieldType *field = (const FieldType *) begin;

	// advance and check size
	begin += sizeof(FieldType) - sizeof(field->name) + ConvertHtoB(field->nameSize);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !ConvertHtoB(field->nameSize) ) // if field has no size, something's up
		return begin;

	Database db;
	db.Number = ConvertHtoB(field->dbNumber);
	db.RecordCount = ConvertHtoB(field->dbRecordCount);
	db.Name.assign((const char *)field->name, ConvertHtoB(field->nameSize) - 1);
	Databases.push_back(db);
	return begin;
}

void DatabaseDatabase::Parse(const Data &data)
{
	// check size to make sure we have up to the DBAccess operation byte
	if( data.GetSize() < (SB_PACKET_DBACCESS_HEADER_SIZE + 1) )
		return;

	MAKE_PACKET(pack, data);
	const unsigned char *begin = 0;
	const unsigned char *end = data.GetData() + data.GetSize();

	switch( pack->u.db.u.response.operation )
	{
	case SB_DBOP_GET_DBDB:
		// using the new protocol
		if( data.GetSize() > SB_PACKET_DBDB_HEADER_SIZE ) {
			begin = (const unsigned char *)
				&pack->u.db.u.response.u.dbdb.field[0];

			// this while check is ok, since ParseField checks
			// for header size
			while( begin < end )
				begin = ParseField<DBDBField>(begin, end);
		}
		else
			dout("DatabaseDatabase: not enough data for parsing");
		break;

	case SB_DBOP_OLD_GET_DBDB:
		// using the old protocol
		if( data.GetSize() > SB_PACKET_OLD_DBDB_HEADER_SIZE ) {
			begin = (const unsigned char *)
				&pack->u.db.u.response.u.old_dbdb.field[0];

			// this while check is ok, since ParseField checks
			// for header size
			while( begin < end )
				begin = ParseField<OldDBDBField>(begin, end);
		}
		else
			dout("DatabaseDatabase: not enough data for parsing");
		break;

	default:
		// unknown protocol
		dout("Unknown protocol");
		break;
	}


}

void DatabaseDatabase::Clear()
{
	Databases.clear();
}

bool DatabaseDatabase::GetDBNumber(const std::string &name,
				   unsigned int &number) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); b++ )
		if( b->Name == name ) {
			number = b->Number;
			return true;
		}
	return false;
}

bool DatabaseDatabase::GetDBName(unsigned int number,
				 std::string &name) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); b++ )
		if( b->Number == number ) {
			name = b->Name;
			return true;
		}
	return false;
}

void DatabaseDatabase::Dump(std::ostream &os) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	os << "Database database:\n";
	for( ; b != Databases.end(); b++ ) {
		os << "    Database: 0x" << setbase(16) << b->Number
		   << " '" << b->Name << "' (records: "
		   << setbase(10) << b->RecordCount << ")\n";
	}
}


std::ostream& operator<< (std::ostream &os, const std::vector<UnknownField> &unknowns)
{
	std::vector<UnknownField>::const_iterator
		ub = unknowns.begin(), ue = unknowns.end();
	if( ub != ue )
		os << "    Unknowns:\n";
	for( ; ub != ue; ub++ ) {
		os << "        Type: 0x" << setbase(16)
		   << (unsigned int) ub->type
		   << " Data:\n" << Data(ub->data.data(), ub->data.size());
	}
	return os;
}



///////////////////////////////////////////////////////////////////////////////
// EmailAddress class

std::ostream& operator<<(std::ostream &os, const EmailAddress &msga) {
	os << msga.Name << " <" << msga.Email << ">";
	return os;
}

std::ostream& operator<<(std::ostream &os, const EmailAddressList &elist) {
	for( EmailAddressList::const_iterator i = elist.begin(); i != elist.end(); ++i ) {
		if( i != elist.begin() )
			os << ", ";
		os << *i;
	}
	return os;
}


///////////////////////////////////////////////////////////////////////////////
// PostalAddress class

//
// GetLabel
//
/// Format a mailing address into a single string, handling missing fields.
///
std::string PostalAddress::GetLabel() const
{
	std::string address = Address1;
	if( Address2.size() ) {
		if( address.size() )
			address += "\n";
		address += Address2;
	}
	if( Address3.size() ) {
		if( address.size() )
			address += "\n";
		address += Address3;
	}
	if( address.size() )
		address += "\n";
	if( City.size() )
		address += City + " ";
	if( Province.size() )
		address += Province + " ";
	if( Country.size() )
		address += Country;
	if( address.size() )
		address += "\n";
	if( PostalCode.size() )
		address += PostalCode;

	return address;
}

void PostalAddress::Clear()
{
	Address1.clear();
	Address2.clear();
	Address3.clear();
	City.clear();
	Province.clear();
	PostalCode.clear();
	Country.clear();
}

std::ostream& operator<<(std::ostream &os, const PostalAddress &post) {
	os << post.GetLabel();
	return os;
}



///////////////////////////////////////////////////////////////////////////////
// Date class

Date::Date(const struct tm *timep)
{
	FromTm(timep);
}

void Date::Clear()
{
	Month = Day = Year = 0;
}

void Date::ToTm(struct tm *timep) const
{
	memset(timep, 0, sizeof(tm));
	timep->tm_year = Year - 1900;
	timep->tm_mon = Month;
	timep->tm_mday = Day;
}

std::string Date::ToYYYYMMDD() const
{
	std::ostringstream oss;
	// setfill and setw not sticky.
	oss	<< setw(4) << setfill('0') << Year
		<< setw(2) << setfill('0') << Month + 1
		<< setw(2) << setfill('0') << Day;
	return oss.str();
}

//
// ToBBString
//
/// The Blackberry stores Birthday and Anniversary date fields
/// with the format: DD/MM/YYYY
///
std::string Date::ToBBString() const
{
	std::ostringstream oss;
	// setw() ain't 'sticky'!
	oss	<< setw(2) << setfill('0') << Day << '/'
		<< setw(2) << setfill('0') << Month + 1 << '/'
		<< setw(2) << setfill('0') << Year;
	return oss.str();
}

bool Date::FromTm(const struct tm *timep)
{
	Year = timep->tm_year + 1900;
	Month = timep->tm_mon;
	Day = timep->tm_mday;
	return true;
}

bool Date::FromBBString(const std::string &str)
{
	int m, d, y;
	if( 3 == sscanf(str.c_str(), "%d/%d/%d", &d, &m, &y) ) {
		Year = y;
		Month = m - 1;
		Day = d;
		return true;
	}
	return false;
}

bool Date::FromYYYYMMDD(const std::string &str)
{
	int m, d, y;
	if( 3 == sscanf(str.c_str(), "%4d%2d%2d", &y, &m, &d) ) {
		Year = y;
		Month = m - 1;
		Day = d;
		return true;
	}
	return false;
}

std::ostream& operator<<(std::ostream &os, const Date &date)
{
	os	<< setw(4) << date.Year << '/'
		<< setw(2) << date.Month << '/'
		<< setw(2) << date.Day;
	return os;
}



} // namespace Barry


#ifdef __TEST_MODE__

#include <iostream>

int main(int argc, char *argv[])
{
	if( argc < 2 ) {
		cerr << "Usage: test <datafile>" << endl;
		return 1;
	}

	std::vector<Data> array;
	if( !LoadDataArray(argv[1], array) ) {
		cerr << "Unable to load file: " << argv[1] << endl;
		return 1;
	}

	cout << "Loaded " << array.size() << " items" << endl;

	for( std::vector<Data>::iterator b = array.begin(), e = array.end();
		b != e; b++ )
	{
		Data &d = *b;
//		cout << d << endl;
		if( d.GetSize() > 13 && d.GetData()[6] == 0x4f ) {
			Barry::Contact contact;
			size_t size = 13;
			contact.ParseFields(d, size);
			cout << contact << endl;
			contact.DumpLdif(cout, "ou=People,dc=example,dc=com");
		}
		else if( d.GetSize() > 13 && d.GetData()[6] == 0x44 ) {
			Barry::Calendar cal;
			size_t size = 13;
			cal.ParseFields(d, size);
			cout << cal << endl;
		}
	}
}

#endif

