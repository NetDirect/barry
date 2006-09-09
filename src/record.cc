///
/// \file	record.cc
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

/*
    Copyright (C) 2005-2006, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "base64.h"
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

std::ostream& operator<<(std::ostream &os, const Message::Address &msga) {
	os << msga.Name.c_str() << " <" << msga.Email.c_str() << ">";
	return os;
}



template <class Record>
const unsigned char*  ParseCommonFields(Record &rec, const void *begin, const void *end)
{
	const unsigned char *b = (const unsigned char*) begin;
	const unsigned char *e = (const unsigned char*) end;

	while( (b + COMMON_FIELD_HEADER_SIZE) < e )
		b = rec.ParseField(b, e);
	return b;
}

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
	size_t strsize = 1;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;
	memcpy(field->u.raw, &c, strsize);

	size += fieldsize;
}

void BuildField(Data &data, size_t &size, uint8_t type, const std::string &str)
{
	// include null terminator
	size_t strsize = str.size() + 1;
	size_t fieldsize = COMMON_FIELD_HEADER_SIZE + strsize;
	unsigned char *pd = data.GetBuffer(size + fieldsize) + size;
	CommonField *field = (CommonField *) pd;

	field->size = htobs(strsize);
	field->type = type;
	memcpy(field->u.raw, str.c_str(), strsize);

	size += fieldsize;
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
	state.Unknown1 = field->unknown;
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
}

void RecordStateTable::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::right);
	char fill = os.fill(' ');
	bool bPrintAscii = Data::PrintAscii();
	Data::PrintAscii(false);

	os << "  Index  RecordId    Dirty" << endl;
	os << "-------  ----------  -----" << endl;

	StateMapType::const_iterator b, e = StateMap.end();
	for( b = StateMap.begin(); b != e ; ++b ) {
		const State &state = b->second;

		os.fill(' ');
		os << setbase(10) << setw(7) << state.Index;
		os << "  0x" << setbase(16) << state.RecordId;
		os << "  " << (state.Dirty ? "yes" : "no");
		os << "  0x" << setbase(16) << state.Unknown1;
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

template <class SizeType>
SizeType ConvertHtoB(SizeType s)
{
	throw BError("Not implemented.");
}

// specializations for specific sizes
template <> uint8_t ConvertHtoB<uint8_t>(uint8_t s)    { return s; }
template <> uint16_t ConvertHtoB<uint16_t>(uint16_t s) { return htobs(s); }
template <> uint32_t ConvertHtoB<uint32_t>(uint32_t s) { return htobl(s); }
template <> uint64_t ConvertHtoB<uint64_t>(uint64_t s) { return htobll(s); }


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

	if( !field->nameSize )		// if field has no size, something's up
		return begin;

	Database db;
	db.Number = field->dbNumber;
	db.RecordCount = field->dbRecordCount;
	db.Name.assign((const char *)field->name, field->nameSize - 1);
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
// Contact class

// Contact field codes
#define CFC_EMAIL		1
#define CFC_PHONE		2
#define CFC_FAX			3
#define CFC_WORK_PHONE		6
#define CFC_HOME_PHONE		7
#define CFC_MOBILE_PHONE	8
#define CFC_PAGER		9
#define CFC_PIN			10
#define CFC_NAME		32	// used twice, in first/last name order
#define CFC_COMPANY		33
#define CFC_DEFAULT_COMM_METHOD	34
#define CFC_ADDRESS1		35
#define CFC_ADDRESS2		36
#define CFC_ADDRESS3		37
#define CFC_CITY		38
#define CFC_PROVINCE		39
#define CFC_POSTAL_CODE		40
#define CFC_COUNTRY		41
#define CFC_TITLE		42
#define CFC_PUBLIC_KEY		43
#define CFC_GROUP_FLAG		44
#define CFC_GROUP_LINK		52
#define CFC_NOTES		64
#define CFC_INVALID_FIELD	255

// Contact code to field table
template <class Record>
struct FieldLink
{
	int type;
	char *name;
	char *ldif;
	char *objectClass;
	std::string Record::* strMember;	// FIXME - find a more general
	Message::Address Record::* addrMember;	// way to do this...
	time_t Record::* timeMember;
};

FieldLink<Contact> ContactFieldLinks[] = {
   { CFC_EMAIL,        "Email",      "mail",0,            &Contact::Email, 0, 0 },
   { CFC_PHONE,        "Phone",      0,0,                 &Contact::Phone, 0, 0 },
   { CFC_FAX,          "Fax",        "facsimileTelephoneNumber",0, &Contact::Fax, 0, 0 },
   { CFC_WORK_PHONE,   "WorkPhone",  "telephoneNumber",0, &Contact::WorkPhone, 0, 0 },
   { CFC_HOME_PHONE,   "HomePhone",  "homePhone",0,       &Contact::HomePhone, 0, 0 },
   { CFC_MOBILE_PHONE, "MobilePhone","mobile",0,          &Contact::MobilePhone, 0, 0 },
   { CFC_PAGER,        "Pager",      "pager",0,           &Contact::Pager, 0, 0 },
   { CFC_PIN,          "PIN",        0,0,                 &Contact::PIN, 0, 0 },
   { CFC_COMPANY,      "Company",    "o",0,               &Contact::Company, 0, 0 },
   { CFC_DEFAULT_COMM_METHOD,"DefaultCommMethod",0,0,     &Contact::DefaultCommunicationsMethod, 0, 0 },
   { CFC_ADDRESS1,     "Address1",   0,0,                 &Contact::Address1, 0, 0 },
   { CFC_ADDRESS2,     "Address2",   0,0,                 &Contact::Address2, 0, 0 },
   { CFC_ADDRESS3,     "Address3",   0,0,                 &Contact::Address3, 0, 0 },
   { CFC_CITY,         "City",       "l",0,               &Contact::City, 0, 0 },
   { CFC_PROVINCE,     "Province",   "st",0,              &Contact::Province, 0, 0 },
   { CFC_POSTAL_CODE,  "PostalCode", "postalCode",0,      &Contact::PostalCode, 0, 0 },
   { CFC_COUNTRY,      "Country",    "c", "country",      &Contact::Country, 0, 0 },
   { CFC_TITLE,        "Title",      "title",0,           &Contact::Title, 0, 0 },
   { CFC_PUBLIC_KEY,   "PublicKey",  0,0,                 &Contact::PublicKey, 0, 0 },
   { CFC_NOTES,        "Notes",      0,0,                 &Contact::Notes, 0, 0 },
   { CFC_INVALID_FIELD,"EndOfList",  0, 0, 0 }
};

Contact::Contact()
	: RecordId(0)
{
}

Contact::~Contact()
{
}

const unsigned char* Contact::ParseField(const unsigned char *begin,
					 const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		if( b->type == field->type ) {
			std::string &s = this->*(b->strMember);
			s.assign((const char *)field->u.raw, field->size-1);
			return begin;	// done!
		}
	}

	// if not found in the type table, check for special handling
	switch( field->type )
	{
	case CFC_NAME: {
		// can be used multiple times, for first/last names
		std::string *name;
		if( FirstName.size() )
			// first name already filled, use last name
			name = &LastName;
		else
			name = &FirstName;

		name->assign((const char*)field->u.raw, field->size-1);
		}
		return begin;

	case CFC_GROUP_LINK:
		// just add the unique ID to the list
		GroupLinks.push_back(
			GroupLink(field->u.link.uniqueId,
				field->u.link.unknown));
		return begin;

	case CFC_GROUP_FLAG:
		// ignore the group flag... the presense of group link items
		// behaves as the flag in this class
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, field->size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Contact::ParseHeader(const Data &data, size_t &offset)
{
	// no header to parse in Contact records
}

// this is called by the RecordParser<> class, which checks size for us
void Contact::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void Contact::BuildHeader(Data &data, size_t &offset) const
{
	// no header in Contact records
}

//
// BuildFields
//
/// Build fields part of record
///
void Contact::BuildFields(Data &data, size_t &offset) const
{
	data.Zap();

	// check if this is a group link record, and if so, output
	// the group flag
	if( GroupLinks.size() )
		BuildField(data, offset, CFC_GROUP_FLAG, 'G');

	// special fields not in type table
	if( FirstName.size() )
		BuildField(data, offset, CFC_NAME, FirstName);
	if( LastName.size() )
		BuildField(data, offset, CFC_NAME, LastName);

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		// print only fields with data
		const std::string &field = this->*(b->strMember);
		if( field.size() ) {
			BuildField(data, offset, b->type, field);
		}
	}

	// save any group links
	GroupLinksType::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	for( ; gb != ge; gb++ ) {
		Barry::Protocol::GroupLink link;
		link.uniqueId = htobl(gb->Link);
		link.unknown = htobs(gb->Unknown);
		BuildField(data, offset, CFC_GROUP_LINK, link);
	}

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, ub->type, ub->data);
	}

	data.ReleaseBuffer(offset);
}

void Contact::Clear()
{
	Email.clear();
	Phone.clear();
	Fax.clear();
	WorkPhone.clear();
	HomePhone.clear();
	MobilePhone.clear();
	Pager.clear();
	PIN.clear();
	FirstName.clear();
	LastName.clear();
	Company.clear();
	DefaultCommunicationsMethod.clear();
	Address1.clear();
	Address2.clear();
	Address3.clear();
	City.clear();
	Province.clear();
	PostalCode.clear();
	Country.clear();
	Title.clear();
	PublicKey.clear();
	Notes.clear();

	GroupLinks.clear();
	Unknowns.clear();
}

//
// GetPostalAddress
//
/// Format a mailing address, handling missing fields.
///
std::string Contact::GetPostalAddress() const
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

void Contact::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::left);
	char fill = os.fill(' ');

	os << "Contact: 0x" << setbase(16) << GetID() << "\n";

	// special fields not in type table
	os << "    " << setw(20) << "FirstName";
	os << ": " << FirstName << "\n";
	os << "    " << setw(20) << "LastName";
	os << ": " << LastName << "\n";

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		// print only fields with data
		const std::string &field = this->*(b->strMember);
		if( field.size() ) {
			os << "    " << setw(20) << b->name;
			os << ": " << field << "\n";
		}
	}

	// print any group links
	GroupLinksType::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	if( gb != ge )
		os << "    GroupLinks:\n";
	for( ; gb != ge; gb++ ) {
		os << "        ID: 0x" << setbase(16) << gb->Link << "\n";
	}

	// and finally print unknowns
	os << Unknowns;

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
}

//
// DumpLdif
//
/// Output contact data to os in LDAP LDIF format.
/// This is hardcoded for now.  Someday we should support mapping
/// of fields.
///
void Contact::DumpLdif(std::ostream &os, const std::string &baseDN) const
{
	ios::fmtflags oldflags = os.setf(ios::left);
	char fill = os.fill(' ');

	if( FirstName.size() == 0 && LastName.size() == 0 )
		return;			// nothing to do

	std::string FullName = FirstName + " " + LastName;
	os << "# Contact 0x" << setbase(16) << GetID() << FullName << "\n";
	os << "dn: " << FullName << "," << baseDN << "\n";
	os << "objectClass: inetOrgPerson\n";
	os << "displayName: " << FullName << "\n";
	os << "cn: " << FullName << "\n";
	if( LastName.size() )
		os << "sn: " << LastName << "\n";
	if( FirstName.size() )
		os << "givenName: " << FirstName << "\n";

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		// print only fields with data
		const std::string &field = this->*(b->strMember);
		if( b->ldif && field.size() ) {
			os << b->ldif << ": " << field << "\n";
			if( b->objectClass )
				os << "objectClass: " << b->objectClass << "\n";
		}
	}

	std::string b64;
	if( Address1.size() && base64_encode(Address1, b64) )
		os << "street:: " << b64 << "\n";

	std::string FullAddress = GetPostalAddress();
	if( FullAddress.size() && base64_encode(FullAddress, b64) )
		os << "postalAddress:: " << b64 << "\n";

	if( Notes.size() && base64_encode(Notes, b64) )
		os << "note:: " << b64 << "\n";

/*
	// print any group links
	GroupLinksType::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	if( gb != ge )
		os << "    GroupLinks:\n";
	for( ; gb != ge; gb++ ) {
		os << "        ID: 0x" << setbase(16) << gb->Link << "\n";
	}
*/

	// last line must be empty
	os << "\n";

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
}

bool Contact::ReadLdif(std::istream &is)
{
	string line;

	// start fresh
	Clear();

	// search for beginning dn: line
	bool found = false;
	while( getline(is, line) ) {
		if( strncmp(line.c_str(), "dn: ", 4) == 0 ) {
			found = true;
			break;
		}
	}
	if( !found )
		return false;

	// storage for various name styles
	string cn, displayName, sn, givenName;
	string coded, decode;
	string *b64field = 0;

	// read ldif lines until empty line is found
	while( getline(is, line) && line.size() ) {

		if( b64field ) {
			// processing a base64 encoded field
			if( line[0] == ' ' ) {
				coded += "\n";
				coded += line;
				continue;
			}
			else {
				// end of base64 block
				base64_decode(coded, decode);
				*b64field = decode;
				coded.clear();
				b64field = 0;
			}
			// fall through to process new line
		}

		if( strncmp(line.c_str(), "cn: ", 4) == 0 ) {
			cn = line.c_str() + 4;	// full name
		}
		else if( strncmp(line.c_str(), "displayName: ", 13) == 0 ) {
			displayName = line.c_str() + 13;	// full name
		}
		else if( strncmp(line.c_str(), "sn: ", 4) == 0 ) {
			sn = line.c_str() + 4;	// last name
		}
		else if( strncmp(line.c_str(), "givenName: ", 11) == 0 ) {
			givenName = line.c_str() + 11;	// first name
		}
		else if( strncmp(line.c_str(), "street:: ", 9) == 0 ) {
			coded = line.substr(9);
			b64field = &Address1;

		}
//		else if( strncmp(line.c_str(), "postalAddress:: ", 16) == 0 ) {
//			std::string FullAddress = GetPostalAddress();
//			if( FullAddress.size() && base64_encode(FullAddress, b64) )
//				os << "postalAddress:: " << b64 << "\n";
//		}
		else if( strncmp(line.c_str(), "note:: ", 7) == 0 ) {
			coded = line.substr(7);
			b64field = &Notes;
		}
		else {

			// cycle through the type table
			for(	FieldLink<Contact> *b = ContactFieldLinks;
				b->type != CFC_INVALID_FIELD;
				b++ )
			{
				// read fields
				if( b->ldif ) {
					std::string &field = this->*(b->strMember);
					std::string key = b->ldif;
					key += ": ";

					if( strncmp(line.c_str(), key.c_str(), key.size()) == 0 ) {
						field = line.c_str() + key.size();
						break;
					}
				}
			}
		}
	}

	if( b64field ) {
		// clean up base64 decoding
		base64_decode(coded, decode);
		*b64field = decode;
		coded.clear();
		b64field = 0;
	}

	// find the best match for name... prefer sn/givenName if available
	if( sn.size() ) {
		LastName = sn;
	}
	if( givenName.size() ) {
		FirstName = givenName;
	}

	if( !LastName.size() || !FirstName.size() ) {
		string first, last;

		// still don't have a complete name, check cn first
		if( cn.size() ) {
			SplitName(cn, first, last);
			if( !LastName.size() && last.size() )
				LastName = last;
			if( !FirstName.size() && first.size() )
				FirstName = first;
		}

		// displayName is last chance
		if( displayName.size() ) {
			SplitName(displayName, first, last);
			if( !LastName.size() && last.size() )
				LastName = last;
			if( !FirstName.size() && first.size() )
				FirstName = first;
		}
	}

	return LastName.size() && FirstName.size();
}

void Contact::SplitName(const std::string &full, std::string &first, std::string &last)
{
	first.clear();
	last.clear();

	string::size_type pos = full.find_last_of(' ');
	if( pos != string::npos ) {
		// has space, assume last word is last name
		last = full.c_str() + pos + 1;
		first = full.substr(0, pos);
	}
	else {
		// no space, assume only first name
		first = full.substr(0);
	}
}



///////////////////////////////////////////////////////////////////////////////
// Message class


// Email / message field codes
#define MFC_TO			0x01		// can occur multiple times
#define MFC_FROM		0x05
#define MFC_SUBJECT		0x0b
#define MFC_BODY		0x0c
#define MFC_END			0xffff

FieldLink<Message> MessageFieldLinks[] = {
   { MFC_TO,      "To",         0, 0,    0, &Message::To, 0 },
   { MFC_FROM,    "From",       0, 0,    0, &Message::From, 0 },
   { MFC_SUBJECT, "Subject",    0, 0,    &Message::Subject, 0, 0 },
   { MFC_BODY,    "Body",       0, 0,    &Message::Body, 0, 0 },
   { MFC_END,     "End of List",0, 0,    0, 0, 0 }
};

Message::Message()
{
}

Message::~Message()
{
}

const unsigned char* Message::ParseField(const unsigned char *begin,
					 const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<Message> *b = MessageFieldLinks;
		b->type != MFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				// parse regular string
				std::string &s = this->*(b->strMember);
				s.assign((const char *)field->u.raw, field->size-1);
				return begin;	// done!
			}
			else if( b->addrMember ) {
				// parse email address
				// get dual name+addr string first
				const char *fa = (const char*)field->u.addr.addr;
				std::string dual(fa, field->size - sizeof(field->u.addr.unknown));

				// assign first string, using null terminator...letting std::string add it for us if it doesn't exist
				Address &a = this->*(b->addrMember);
				a.Name = dual.c_str();

				// assign second string, using first size as starting point
				a.Email = dual.c_str() + a.Name.size() + 1;
			}
		}
	}

	return begin;
}

// empty API, not required by protocol
uint32_t Message::GetUniqueId() const
{
	throw std::logic_error("Message::GetUniqueId() called, and not supported by the USB protocol.  Should never get called.");
}

// empty API, not required by protocol
void Message::SetUniqueId(uint32_t Id)
{
	// accept it without complaining, just do nothing
}

void Message::ParseHeader(const Data &data, size_t &offset)
{
	// we skip the "header" since we don't know what to do with it yet
	offset += MESSAGE_RECORD_HEADER_SIZE;
}

void Message::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void Message::BuildHeader(Data &data, size_t &offset) const
{
	throw std::logic_error("Message::BuildHeader not yet implemented");
}

void Message::BuildFields(Data &data, size_t &offset) const
{
	throw std::logic_error("Message::BuildFields not yet implemented");
}

void Message::Clear()
{
	From.Name.clear();
	From.Email.clear();
	To.Name.clear();
	To.Email.clear();
	Cc.Name.clear();
	Cc.Email.clear();
	Subject.clear();
	Body.clear();
}

// dump message in mbox format
void Message::Dump(std::ostream &os) const
{
	// FIXME - use current time until we figure out the date headers
	time_t fixme = time(NULL);

	os << "From " << (From.Email.size() ? From.Email.c_str() : "unknown")
	   << "  " << ctime(&fixme);
	os << "Date: " << ctime(&fixme);
	os << "From: " << From << "\n";
	if( To.Email.size() )
		os << "To: " << To << "\n";
	if( Cc.Email.size() )
		os << "Cc: " << Cc << "\n";
	if( Subject.size() )
		os << "Subject: " << Subject << "\n";
	os << "\n";
	for(	std::string::const_iterator i = Body.begin();
		i != Body.end() && *i;
		i++)
	{
		if( *i == '\r' )
			os << '\n';
		else
			os << *i;
	}
	os << "\n\n";
}



///////////////////////////////////////////////////////////////////////////////
// Calendar class

// calendar field codes
#define CALFC_APPT_TYPE_FLAG		0x01
#define CALFC_SUBJECT			0x02
#define CALFC_NOTES			0x03
#define CALFC_LOCATION			0x04
#define CALFC_NOTIFICATION_TIME		0x05
#define CALFC_START_TIME		0x06
#define CALFC_END_TIME			0x07
#define CALFC_RECURRANCE_DATA		0x0c
#define CALFC_VERSION_DATA		0x10
#define CALFC_NOTIFICATION_DATA		0x1a
#define CALFC_ALLDAYEVENT_FLAG		0xff
#define CALFC_END			0xffff

FieldLink<Calendar> CalendarFieldLinks[] = {
   { CALFC_SUBJECT,    "Subject",    0, 0,    &Calendar::Subject, 0, 0 },
   { CALFC_NOTES,      "Notes",      0, 0,    &Calendar::Notes, 0, 0 },
   { CALFC_LOCATION,   "Location",   0, 0,    &Calendar::Location, 0, 0 },
   { CALFC_NOTIFICATION_TIME,"Notification Time",0,0, 0, 0, &Calendar::NotificationTime },
   { CALFC_START_TIME, "Start Time", 0, 0,    0, 0, &Calendar::StartTime },
   { CALFC_END_TIME,   "End Time",   0, 0,    0, 0, &Calendar::EndTime },
   { CALFC_END,        "End of List",0, 0,    0, 0, 0 }
};

Calendar::Calendar()
	: Recurring(false),
	AllDayEvent(false)
{
	Clear();
}

Calendar::~Calendar()
{
}

const unsigned char* Calendar::ParseField(const unsigned char *begin,
					  const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s.assign((const char *)field->u.raw, field->size-1);
				return begin;	// done!
			}
			else if( b->timeMember ) {
				time_t &t = this->*(b->timeMember);
				t = min2time(field->u.min1900);
				return begin;
			}
		}
	}

	// handle special cases
	switch( field->type )
	{
	case CALFC_APPT_TYPE_FLAG:
		switch( field->u.raw[0] )
		{
		case 'a':			// regular non-recurring appointment
			Recurring = false;
			return begin;

		case '*':			// recurring appointment
			Recurring = true;
			return begin;

		default:
			throw BError("Calendar::ParseField: unknown appointment type");
		}
		break;

	case CALFC_ALLDAYEVENT_FLAG:
		AllDayEvent = field->u.raw[0] == 1;
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->u.raw, field->size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void Calendar::ParseHeader(const Data &data, size_t &offset)
{
	// no header in Calendar records
}

void Calendar::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
	offset += finish - (data.GetData() + offset);
}

void Calendar::BuildHeader(Data &data, size_t &offset) const
{
	// no header in Calendar records
}

//
// Build
//
/// Build fields part of record.
///
void Calendar::BuildFields(Data &data, size_t &offset) const
{
	data.Zap();

	// output the type first
	BuildField(data, offset, CALFC_APPT_TYPE_FLAG, Recurring ? '*' : 'a');

	// output all day event flag only if set
	if( AllDayEvent )
		BuildField(data, offset, CALFC_ALLDAYEVENT_FLAG, (char)1);

	// cycle through the type table
	for(	const FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
		b++ )
	{
		if( b->strMember ) {
			const std::string &s = this->*(b->strMember);
			if( s.size() )
				BuildField(data, offset, b->type, s);
		}
		else if( b->timeMember ) {
			time_t t = this->*(b->timeMember);
			if( t > 0 )
				BuildField1900(data, offset, b->type, t);
		}
	}

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, ub->type, ub->data);
	}

	data.ReleaseBuffer(offset);
}

void Calendar::Clear()
{
	Subject.clear();
	Notes.clear();
	Location.clear();
	NotificationTime = StartTime = EndTime = 0;
	Unknowns.clear();
}

void Calendar::Dump(std::ostream &os) const
{
	os << "Calendar entry: 0x" << setbase(16) << RecordId << "\n";
	os << "   Recurring: " << (Recurring ? "yes" : "no") << "\n";
	os << "   All Day Event: " << (AllDayEvent ? "yes" : "no") << "\n";

	// cycle through the type table
	for(	const FieldLink<Calendar> *b = CalendarFieldLinks;
		b->type != CALFC_END;
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

	// print any unknowns
	os << Unknowns;
}


///////////////////////////////////////////////////////////////////////////////
// ServiceBookConfig class

// service book packed field codes
#define SBFCC_END			0xffff

FieldLink<ServiceBookConfig> ServiceBookConfigFieldLinks[] = {
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
						   const unsigned char *end)
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
				s.assign((const char *)raw, size-1);
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

void ServiceBookConfig::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
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
void ServiceBookConfig::BuildFields(Data &data, size_t &offset) const
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

FieldLink<ServiceBook> ServiceBookFieldLinks[] = {
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
					  const unsigned char *end)
{
	const CommonField *field = (const CommonField *) begin;

	// advance and check size
	begin += COMMON_FIELD_HEADER_SIZE + field->size;
	if( begin > end )		// if begin==end, we are ok
		return begin;

	if( !field->size )		// if field has no size, something's up
		return begin;

	// cycle through the type table
	for(	FieldLink<ServiceBook> *b = ServiceBookFieldLinks;
		b->type != SBFC_END;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s.assign((const char *)field->u.raw, field->size-1);
				return begin;	// done!
			}
			else if( b->timeMember ) {
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
		Name.assign((const char *)field->u.raw, field->size-1);
		NameType = field->type;
		return begin;

	case SBFC_OLD_DESC:
	case SBFC_DESCRIPTION:
		Description.assign((const char *)field->u.raw, field->size-1);
		DescType = field->type;
		return begin;

	case SBFC_OLD_UNIQUE_ID:
	case SBFC_UNIQUE_ID:
		UniqueId.assign((const char *)field->u.raw, field->size);
		UniqueIdType = field->type;
		return begin;

	case SBFC_CONTENT_ID:
		ContentId.assign((const char *)field->u.raw, field->size);
		return begin;

	case SBFC_BES_DOMAIN:
		BesDomain.assign((const char *)field->u.raw, field->size);
		return begin;

	case SBFC_CONFIG:
		{
			Data config((const void *)field->u.raw, field->size);
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
	uf.data.assign((const char*)field->u.raw, field->size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

void ServiceBook::ParseHeader(const Data &data, size_t &offset)
{
	// no header in this record (?)
}

void ServiceBook::ParseFields(const Data &data, size_t &offset)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize());
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
void ServiceBook::BuildFields(Data &data, size_t &offset) const
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
	os << "ServiceBook entry: 0x" << setbase(16) << RecordId << "\n";

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

