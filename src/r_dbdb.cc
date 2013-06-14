///
/// \file	r_dbdb.cc
///		DatabaseDatabase record parser class
///

/*
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "i18n.h"
#include "record.h"
#include "record-internal.h"
#include "data.h"
#include "protocol.h"
#include "debug.h"
#include <algorithm>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

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
			dout(_("DatabaseDatabase: not enough data for parsing"));
		break;

	default:
		// unknown protocol
		dout(_("Unknown protocol"));
		break;
	}


}

void DatabaseDatabase::Clear()
{
	Databases.clear();
}

namespace {
	bool NameSort(const DatabaseDatabase::Database &one,
		const DatabaseDatabase::Database &two)
	{
		return one.Name < two.Name;
	}

	bool CountSort(const DatabaseDatabase::Database &one,
		const DatabaseDatabase::Database &two)
	{
		return one.RecordCount < two.RecordCount;
	}
}

void DatabaseDatabase::SortByName()
{
	std::sort(Databases.begin(), Databases.end(), NameSort);
}

void DatabaseDatabase::SortByRecordCount()
{
	std::sort(Databases.begin(), Databases.end(), CountSort);
}

unsigned int DatabaseDatabase::GetTotalRecordCount() const
{
	unsigned int sum = 0;

	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); ++b ) {
		sum += b->RecordCount;
	}
	return sum;
}

bool DatabaseDatabase::GetDBNumber(const std::string &name,
				   unsigned int &number) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); ++b )
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
	for( ; b != Databases.end(); ++b )
		if( b->Number == number ) {
			name = b->Name;
			return true;
		}
	return false;
}

void DatabaseDatabase::Dump(std::ostream &os) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	os << _("Database database:\n");
	for( ; b != Databases.end(); b++ ) {
		os << _("    Database: ") << "0x" << setbase(16) << b->Number
		   << " '" << b->Name << "' (" << _("records: ")
		   << setbase(10) << b->RecordCount << ")\n";
	}
}

} // namespace Barry

