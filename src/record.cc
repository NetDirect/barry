///
/// \file	record.cc
///		Blackberry database record classes.  Help translate data
///		from data packets to useful structurs, and back.
///

/*
    Copyright (C) 2005, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "data.h"
#include <ostream>
#include <iomanip>
#include <time.h>

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;

namespace Barry {

std::ostream& operator<<(std::ostream &os, const Message::Address &msga) {
	os << msga.Name.c_str() << " <" << msga.Email.c_str() << ">";
	return os;
}



template <class Record>
void ParseCommonFields(Record &rec, const void *begin, const void *end)
{
	const unsigned char *b = (const unsigned char*) begin;
	const unsigned char *e = (const unsigned char*) end;

	while( (b + COMMON_FIELD_HEADER_SIZE) < e )
		b = rec.ParseField(b, e);
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
	begin += COMMAND_FIELD_HEADER_SIZE + field->size;
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

void CommandTable::Parse(const Data &data, int offset)
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
	begin += sizeof(FieldType) - sizeof(field->name) + field->nameSize;
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
	if( (unsigned int)data.GetSize() < (SB_PACKET_DBACCESS_HEADER_SIZE + 1) )
		return;

	MAKE_PACKET(pack, data);
	const unsigned char *begin = 0;
	const unsigned char *end = data.GetData() + data.GetSize();

	switch( pack->data.db.data.dbdb.operation )
	{
	case SB_DBOP_GET_DBDB:
		// using the new protocol
		if( (unsigned int)data.GetSize() > SB_PACKET_DBDB_HEADER_SIZE ) {
			begin = (const unsigned char *)
				&pack->data.db.data.dbdb.field[0];

			// this while check is ok, since ParseField checks
			// for header size
			while( begin < end )
				begin = ParseField<DBDBField>(begin, end);
		}
		else
			dout("Contact: not enough data for parsing");
		break;

	case SB_DBOP_OLD_GET_DBDB:
		// using the old protocol
		if( (unsigned int)data.GetSize() > SB_PACKET_OLD_DBDB_HEADER_SIZE ) {
			begin = (const unsigned char *)
				&pack->data.db.data.old_dbdb.field[0];

			// this while check is ok, since ParseField checks
			// for header size
			while( begin < end )
				begin = ParseField<OldDBDBField>(begin, end);
		}
		else
			dout("Contact: not enough data for parsing");
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

unsigned int DatabaseDatabase::GetDBNumber(const std::string &name) const
{
	DatabaseArrayType::const_iterator b = Databases.begin();
	for( ; b != Databases.end(); b++ )
		if( b->Name == name )
			return b->Number;
	return 0;
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
#define CFC_GROUP_LINK		52
#define CFC_NOTES		64
#define CFC_INVALID_FIELD	255

// Contact code to field table
template <class Record>
struct FieldLink
{
	int type;
	char *name;
	std::string Record::* strMember;
	Message::Address Record::* addrMember;
};

FieldLink<Contact> ContactFieldLinks[] = {
   { CFC_EMAIL,        "Email",      &Contact::Email, 0 },
   { CFC_PHONE,        "Phone",      &Contact::Phone, 0 },
   { CFC_FAX,          "Fax",        &Contact::Fax, 0 },
   { CFC_WORK_PHONE,   "WorkPhone",  &Contact::WorkPhone, 0 },
   { CFC_HOME_PHONE,   "HomePhone",  &Contact::HomePhone, 0 },
   { CFC_MOBILE_PHONE, "MobilePhone",&Contact::MobilePhone, 0 },
   { CFC_PAGER,        "Pager",      &Contact::Pager, 0 },
   { CFC_PIN,          "PIN",        &Contact::PIN, 0 },
   { CFC_COMPANY,      "Company",    &Contact::Company, 0 },
   { CFC_DEFAULT_COMM_METHOD,"DefaultCommMethod",&Contact::DefaultCommunicationsMethod, 0 },
   { CFC_ADDRESS1,     "Address1",   &Contact::Address1, 0 },
   { CFC_ADDRESS2,     "Address2",   &Contact::Address2, 0 },
   { CFC_ADDRESS3,     "Address3",   &Contact::Address3, 0 },
   { CFC_CITY,         "City",       &Contact::City, 0 },
   { CFC_PROVINCE,     "Province",   &Contact::Province, 0 },
   { CFC_POSTAL_CODE,  "PostalCode", &Contact::PostalCode, 0 },
   { CFC_COUNTRY,      "Country",    &Contact::Country, 0 },
   { CFC_TITLE,        "Title",      &Contact::Title, 0 },
   { CFC_PUBLIC_KEY,   "PublicKey",  &Contact::PublicKey, 0 },
   { CFC_NOTES,        "Notes",      &Contact::Notes, 0 },
   { CFC_INVALID_FIELD,"EndOfList",  0, 0 }
};

Contact::Contact()
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
			s.assign((const char *)field->data.raw, field->size-1);
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

		name->assign((const char*)field->data.raw, field->size-1);
		}
		return begin;

	case CFC_GROUP_LINK:
		// just add the unique ID to the list
		GroupLinks.push_back(field->data.link.uniqueId);
		return begin;
	}

	// if still not handled, add to the Unknowns list
	UnknownField uf;
	uf.type = field->type;
	uf.data.assign((const char*)field->data.raw, field->size);
	Unknowns.push_back(uf);

	// return new pointer for next field
	return begin;
}

// this is called by the RecordParser<> class, which checks size for us
void Contact::Parse(const Data &data, unsigned int operation)
{
	MAKE_PACKET(pack, data);
	const void *begin = 0;
	switch( operation )
	{
	case SB_DBOP_GET_RECORDS:
		// using the new protocol
		// save the contact record ID
		m_recordId = pack->data.db.data.contact.uniqueId;
		begin = &pack->data.db.data.contact.field[0];
		break;

	case SB_DBOP_OLD_GET_RECORDS_REPLY:
		// using the old protocol
		// save the contact record ID
		m_recordId = pack->data.db.data.old_contact.uniqueId;
		begin = &pack->data.db.data.old_contact.field[0];
		break;
	}

	ParseCommonFields(*this, begin, data.GetData() + data.GetSize());
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
	std::vector<uint64_t>::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	if( gb != ge )
		os << "    GroupLinks:\n";
	for( ; gb != ge; gb++ ) {
		os << "        ID: 0x" << setbase(16) << *gb << "\n";
	}

	// and finally print unknowns
	std::vector<UnknownField>::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	if( ub != ue )
		os << "    Unknowns:\n";
	for( ; ub != ue; ub++ ) {
		os << "        Type: 0x" << setbase(16)
		   << (unsigned int) ub->type
		   << " Data:\n" << Data(ub->data.data(), ub->data.size());
	}

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
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
   { MFC_TO,      "To",         0, &Message::To },
   { MFC_FROM,    "From",       0, &Message::From },
   { MFC_SUBJECT, "Subject",    &Message::Subject, 0 },
   { MFC_BODY,    "Body",       &Message::Body, 0 },
   { MFC_END,     "End of List", 0, 0 }
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
				s.assign((const char *)field->data.raw, field->size-1);
				return begin;	// done!
			}
			else if( b->addrMember ) {
				// parse email address
				// get dual name+addr string first
				const char *fa = (const char*)field->data.addr.addr;
				std::string dual(fa, field->size - sizeof(field->data.addr.unknown));

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

void Message::Parse(const Data &data, unsigned int operation)
{
	MAKE_PACKET(pack, data);
	const void *begin = 0;
	switch( operation )
	{
	case SB_DBOP_GET_RECORDS:
		begin = &pack->data.db.data.message.field[0];
		break;

	case SB_DBOP_OLD_GET_RECORDS_REPLY:
		begin = &pack->data.db.data.old_message.field[0];
		break;
	}

	ParseCommonFields(*this, begin, data.GetData() + data.GetSize());
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

	for( std::vector<Data>::iterator b = array.begin(), e = array.end();
		b != e; b++ )
	{
		Data &d = *b;
//		cout << d << endl;
		if( d.GetSize() > 13 && d.GetData()[6] == 0x4f ) {
			Barry::Contact contact;
			contact.Parse(d, d.GetData()[6]);
			cout << contact << endl;
		}
	}
}

#endif

