///
/// \file	r_folder.cc
///		Record parsing class for the folders database.
///

/*
    Copyright (C) 2005-2007, Net Direct Inc. (http://www.netdirect.ca/)
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

#include "r_folder.h"
#include "record-internal.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "debug.h"
#include <ostream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// Folder Class

// Folder Field Codes

#define FFC_NUMBER	0x0a
#define FFC_LEVEL	0x0b
#define FFC_NAME	0x0c
#define FFC_ADDRESS1	0x0d
#define FFC_ADDRESS2	0x0e
#define FFC_TYPE	0x0f
#define FFC_END	0xffff

// Folder Types
#define SUBTREE	0x00
#define DELETED	0x01
#define INBOX		0x02
#define OUTBOX		0x03
#define SENT		0x04
#define OTHER		0x05
#define DRAFT		0x0a

// Folder Status
#define ORPHAN		0x50
#define UNFILED	0x51
#define FILED		0x52

#define INVALID 	-1

#define SEPARATOR	0x2f
#define ROOT_SEPARATOR	0x3a

FieldLink<Folder> FolderFieldLinks[] = {
	{ FFC_NAME,      "FolderName",   0, 0, &Folder::FolderName, 0, 0 },
	{ FFC_END,       "End of List",  0, 0, 0, 0, 0 },
};

Folder::Folder()
{
	Clear();
}


Folder::~Folder()
{
}

const unsigned char* Folder::ParseField(const unsigned char *begin,
				      const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
	if( begin > end )       // if begin==end, we are ok
		return begin;

	if( !btohs(field->size) )   // if field has no size, something's up
		return begin;
	
	// cycle through the type table
	for(    FieldLink<Folder> *b = FolderFieldLinks;
		b->type != FFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				return begin;   // done!
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
	case FFC_TYPE:
		FolderType = (FolderTypeEnum)field->u.raw[0];
		return begin;
	case FFC_NUMBER:
		FolderNumber = field->u.raw[0];	// two's complement
		return begin;
	case FFC_LEVEL:
		FolderLevel = field->u.raw[0];
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, btohs(field->size));
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Folder::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Folder records
}

void Folder::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
	data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void Folder::Clear()
{
	FolderName.clear();
	Unknowns.clear();
	FolderType = FolderSubtree;
}

void Folder::Dump(std::ostream &os) const
{
	static const char *FolderTypeString[] = { "Subtree", "Deleted", "Inbox", "Outbox", "Sent", "Other"};
//	static const char *FolderStatusString[] = { "Orphan", "Unfiled", "Filed" };
	
	os << "Folder Records\n\n";
	os << "Folder Name: " << FolderName << "\n";
	os << "Folder Type: ";
	if( FolderType < FolderDraft )
		os << FolderTypeString[FolderType] << "\n";
	else if( FolderType == FolderDraft )
		os << "Draft\n";
	else
		os << "Unknown (" << std::hex << FolderType << ")\n";
	os << "Folder Number: " << std::dec << FolderNumber << "\n";
	os << "Folder Level: " << std::dec << FolderLevel << "\n";
	os << "\n";
	os << Unknowns;
	os << "\n\n";
}

} // namespace Barry

