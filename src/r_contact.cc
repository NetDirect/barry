///
/// \file	r_contact.cc
///		Blackberry database record parser class for contact records.
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

#include "r_contact.h"
#include "record-internal.h"
#include "protocol.h"
#include "protostructs.h"
#include "data.h"
#include "time.h"
#include "error.h"
#include "endian.h"
#include "iconv.h"
#include "trim.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <stdexcept>
#include "ios_state.h"

#define __DEBUG_MODE__
#include "debug.h"

using namespace std;
using namespace Barry::Protocol;

namespace Barry {



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
#define CFC_RADIO		14	// 0x0e
#define CFC_WORK_PHONE_2	16	// 0x10
#define CFC_HOME_PHONE_2	17	// 0x11
#define CFC_OTHER_PHONE		18	// 0x12
#define CFC_MOBILE_PHONE_2	19	// 0x13
#define CFC_HOME_FAX		20	// 0x14
#define CFC_NAME		32	// 0x20 used twice, in first/last name order
#define CFC_COMPANY		33
#define CFC_DEFAULT_COMM_METHOD	34
#define CFC_ADDRESS1		35
#define CFC_ADDRESS2		36
#define CFC_ADDRESS3		37
#define CFC_CITY		38
#define CFC_PROVINCE		39
#define CFC_POSTAL_CODE		40
#define CFC_COUNTRY		41
#define CFC_TITLE		42	// 0x2a
#define CFC_PUBLIC_KEY		43
#define CFC_GROUP_FLAG		44
#define CFC_GROUP_LINK		52
#define CFC_URL			54	// 0x36
#define CFC_PREFIX		55	// 0x37
#define CFC_CATEGORY		59	// 0x3B
#define CFC_HOME_ADDRESS1	61	// 0x3D
#define CFC_HOME_ADDRESS2	62	// 0x3E
  // If the address 3 isn't mapped then it appears
  // in the same field as address2 with a space
#define CFC_HOME_ADDRESS3	63 	// 0x3F
#define CFC_NOTES		64	// 0x40
#define CFC_USER_DEFINED_1	65	// 0x41
#define CFC_USER_DEFINED_2	66	// 0x42
#define CFC_USER_DEFINED_3	67	// 0x43
#define CFC_USER_DEFINED_4	68	// 0x44
#define CFC_HOME_CITY		69	// 0x45
#define CFC_HOME_PROVINCE	70	// 0x46
#define CFC_HOME_POSTAL_CODE	71	// 0x47
#define CFC_HOME_COUNTRY	72	// 0x48
#define CFC_IMAGE		77	// 0x4D
#define CFC_BIRTHDAY		82	// 0x52
#define CFC_ANNIVERSARY		83	// 0x53
#define CFC_MAYBE_CATEGORYID	84	// 0x54
#define CFC_UNIQUEID		85	// 0x55
#define CFC_NICKNAME		86	// 0x56
#define CFC_INVALID_FIELD	255

// Contact code to field table
static FieldLink<Contact> ContactFieldLinks[] = {
   { CFC_NICKNAME,     "Nickname",   0,0,                 &Contact::Nickname, 0, 0, 0, 0, true },
   { CFC_PHONE,        "Phone",      0,0,                 &Contact::Phone, 0, 0, 0, 0, true },
   { CFC_FAX,          "Fax",        "facsimileTelephoneNumber",0, &Contact::Fax, 0, 0, 0, 0, true },
   { CFC_HOME_FAX,     "HomeFax",    0,0,                 &Contact::HomeFax, 0, 0, 0, 0, true },
   { CFC_WORK_PHONE,   "WorkPhone",  "telephoneNumber",0, &Contact::WorkPhone, 0, 0, 0, 0, true },
   { CFC_HOME_PHONE,   "HomePhone",  "homePhone",0,       &Contact::HomePhone, 0, 0, 0, 0, true },
   { CFC_MOBILE_PHONE, "MobilePhone","mobile",0,          &Contact::MobilePhone, 0, 0, 0, 0, true },
   { CFC_MOBILE_PHONE_2,"MobilePhone2",0,0,               &Contact::MobilePhone2, 0, 0, 0, 0, true },
   { CFC_PAGER,        "Pager",      "pager",0,           &Contact::Pager, 0, 0, 0, 0, true },
   { CFC_PIN,          "PIN",        0,0,                 &Contact::PIN, 0, 0, 0, 0, true },
   { CFC_RADIO,        "Radio",      0,0,                 &Contact::Radio, 0, 0, 0, 0, true },
   { CFC_WORK_PHONE_2, "WorkPhone2", 0,0,                 &Contact::WorkPhone2, 0, 0, 0, 0, true },
   { CFC_HOME_PHONE_2, "HomePhone2", 0,0,                 &Contact::HomePhone2, 0, 0, 0, 0, true },
   { CFC_OTHER_PHONE,  "OtherPhone", 0,0,                 &Contact::OtherPhone, 0, 0, 0, 0, true },
   { CFC_COMPANY,      "Company",    "o",0,               &Contact::Company, 0, 0, 0, 0, true },
   { CFC_DEFAULT_COMM_METHOD,"DefaultCommMethod",0,0,     &Contact::DefaultCommunicationsMethod, 0, 0, 0, 0, true },
   { CFC_ADDRESS1,     "WorkAddress1",   0,0,             0, 0, 0, &Contact::WorkAddress, &PostalAddress::Address1, true },
   { CFC_ADDRESS2,     "WorkAddress2",   0,0,             0, 0, 0, &Contact::WorkAddress, &PostalAddress::Address2, true },
   { CFC_ADDRESS3,     "WorkAddress3",   0,0,             0, 0, 0, &Contact::WorkAddress, &PostalAddress::Address3, true },
   { CFC_CITY,         "WorkCity",       "l",0,           0, 0, 0, &Contact::WorkAddress, &PostalAddress::City, true },
   { CFC_PROVINCE,     "WorkProvince",   "st",0,          0, 0, 0, &Contact::WorkAddress, &PostalAddress::Province, true },
   { CFC_POSTAL_CODE,  "WorkPostalCode", "postalCode",0,  0, 0, 0, &Contact::WorkAddress, &PostalAddress::PostalCode, true },
   { CFC_COUNTRY,      "WorkCountry",    "c", "country",  0, 0, 0, &Contact::WorkAddress, &PostalAddress::Country, true },
   { CFC_TITLE,        "JobTitle",   "title",0,           &Contact::JobTitle, 0, 0, 0, 0, true },
   { CFC_PUBLIC_KEY,   "PublicKey",  0,0,                 &Contact::PublicKey, 0, 0, 0, 0, false },
   { CFC_URL,          "URL",        0,0,                 &Contact::URL, 0, 0, 0, 0, true },
   { CFC_PREFIX,       "Prefix",     0,0,                 &Contact::Prefix, 0, 0, 0, 0, true },
   { CFC_HOME_ADDRESS1,"HomeAddress1", 0,0,               0, 0, 0, &Contact::HomeAddress, &PostalAddress::Address1, true },
   { CFC_HOME_ADDRESS2,"HomeAddress2", 0,0,               0, 0, 0, &Contact::HomeAddress, &PostalAddress::Address2, true },
   { CFC_HOME_ADDRESS3,"HomeAddress3", 0,0,               0, 0, 0, &Contact::HomeAddress, &PostalAddress::Address3, true },
   { CFC_NOTES,        "Notes",      0,0,                 &Contact::Notes, 0, 0, 0, 0, true },
   { CFC_USER_DEFINED_1, "UserDefined1", 0,0,             &Contact::UserDefined1, 0, 0, 0, 0, true },
   { CFC_USER_DEFINED_2, "UserDefined2", 0,0,             &Contact::UserDefined2, 0, 0, 0, 0, true },
   { CFC_USER_DEFINED_3, "UserDefined3", 0,0,             &Contact::UserDefined3, 0, 0, 0, 0, true },
   { CFC_USER_DEFINED_4, "UserDefined4", 0,0,             &Contact::UserDefined4, 0, 0, 0, 0, true },
   { CFC_HOME_CITY,    "HomeCity",   0,0,                 0, 0, 0, &Contact::HomeAddress, &PostalAddress::City, true },
   { CFC_HOME_PROVINCE,"HomeProvince", 0,0,               0, 0, 0, &Contact::HomeAddress, &PostalAddress::Province, true },
   { CFC_HOME_POSTAL_CODE, "HomePostalCode", 0,0,         0, 0, 0, &Contact::HomeAddress, &PostalAddress::PostalCode, true },
   { CFC_HOME_COUNTRY, "HomeCountry",0,0,                 0, 0, 0, &Contact::HomeAddress, &PostalAddress::Country, true },
   { CFC_IMAGE,        "Image",      0,0,                 &Contact::Image, 0, 0, 0, 0, false },
   { CFC_INVALID_FIELD,"EndOfList",  0, 0, 0, 0, 0, 0, 0, false }
};

Contact::Contact()
	: RecType(Contact::GetDefaultRecType()),
	RecordId(0),
	m_FirstNameSeen(false)
{
}

Contact::~Contact()
{
}

const unsigned char* Contact::ParseField(const unsigned char *begin,
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
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		if( b->type == field->type ) {
			if( b->strMember ) {
				std::string &s = this->*(b->strMember);
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return begin;	// done!
			}
			else if( b->postMember && b->postField ) {
				std::string &s = (this->*(b->postMember)).*(b->postField);
				s = ParseFieldString(field);
				if( b->iconvNeeded && ic )
					s = ic->FromBB(s);
				return begin;
			}
			else {
				break;	// fall through to special handling
			}
		}
	}

	// if not found in the type table, check for special handling
	switch( field->type )
	{
	case CFC_EMAIL: {
		std::string s = ParseFieldString(field);
		if( ic )
			s = ic->FromBB(s);
		EmailAddresses.push_back( s );
		}
		return begin;

	case CFC_NAME: {
		// can be used multiple times, for first/last names
		std::string *name;
		if( FirstName.size() || m_FirstNameSeen ) {
			// first name already filled, use last name
			name = &LastName;
			m_FirstNameSeen = false;
		}
		else {
			name = &FirstName;
			m_FirstNameSeen = true;
		}

		*name = ParseFieldString(field);
		if( ic )
			*name = ic->FromBB(*name);
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

	case CFC_CATEGORY: {
		std::string catstring = ParseFieldString(field);
		if( ic )
			catstring = ic->FromBB(catstring);
		Categories.CategoryStr2List(catstring);
		}
		return begin;

	case CFC_BIRTHDAY: {
		std::string bstring = ParseFieldString(field);
		Birthday.FromBBString(bstring);
		}
		return begin;

	case CFC_ANNIVERSARY: {
		std::string astring = ParseFieldString(field);
		Anniversary.FromBBString(astring);
		}
		return begin;

	case CFC_UNIQUEID:
		// this is a duplicate of the UniqueID that comes from
		// the envelope part of the protocol... just throw this
		// away, since when we upload it, we need to use a
		// consistent UniqueID / RecordID from the API
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

void Contact::ParseHeader(const Data &data, size_t &offset)
{
	// no header to parse in Contact records
}

void Contact::ParseFields(const Data &data, size_t &offset, const IConverter *ic)
{
	const unsigned char *finish = ParseCommonFields(*this,
		data.GetData() + offset, data.GetData() + data.GetSize(), ic);
	offset += finish - (data.GetData() + offset);
}

void Contact::Validate() const
{
	if( !GetFullName().size() && !Company.size() ) {
		throw Barry::ValidationError("A contact record must contain either a First/Last name, or a Company name.");
	}
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
void Contact::BuildFields(Data &data, size_t &offset, const IConverter *ic) const
{
	data.Zap();

	// Sanity check: the Blackberry requires at least a name or
	// a company name for each address record.
	if( !GetFullName().size() && !Company.size() )
		throw BadData("Contact must have name or company name.");

	// check if this is a group link record, and if so, output
	// the group flag
	if( GroupLinks.size() )
		BuildField(data, offset, CFC_GROUP_FLAG, 'G');

	// special fields not in type table
	if( FirstName.size() ) {
		std::string s = ic ? ic->ToBB(FirstName) : FirstName;
		BuildField(data, offset, CFC_NAME, s);
	}
	if( LastName.size() ) {
		if( !FirstName.size() ) {
			// order matters with first/last name, and if
			// last name exists, and first name doesn't,
			// insert blank first name ahead of it
			BuildField(data, offset, CFC_NAME, "");
		}
		BuildField(data, offset, CFC_NAME, ic ? ic->ToBB(LastName) : LastName);
	}

//	FIXME
//	// add unknown data
//	char buffer[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
//	BuildField(data, offset, 0x54, buffer, 8);

	// With the BlackBerry Storm, I have to add this entry.
	// Otherwise the uniqueId of this contact is reseted !
	// The device seems accept the multiple contact with the same uniqueId,
	// but the synchronization process uses this uniqueId to identify the contact.
	// add uniqueId
	BuildField(data, offset, CFC_UNIQUEID, RecordId);

	// add all email addresses
	EmailList::const_iterator eai = EmailAddresses.begin();
	for( ; eai != EmailAddresses.end(); ++eai ) {
		if( eai->size() ) {
			BuildField(data, offset, CFC_EMAIL, ic ? ic->ToBB(*eai) : *eai);
		}
	}

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		// print only fields with data
		if( b->strMember ) {
			const std::string &field = this->*(b->strMember);
			if( field.size() ) {
				std::string s = (b->iconvNeeded && ic) ? ic->ToBB(field) : field;
				BuildField(data, offset, b->type, s);
			}
		}
		else if( b->postMember && b->postField ) {
			const std::string &field = (this->*(b->postMember)).*(b->postField);
			if( field.size() ) {
				std::string s = (b->iconvNeeded && ic) ? ic->ToBB(field) : field;
				BuildField(data, offset, b->type, s);
			}
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

	// save categories
	if( Categories.size() ) {
		string store;
		Categories.CategoryList2Str(store);
		BuildField(data, offset, CFC_CATEGORY, ic ? ic->ToBB(store) : store);
	}

	// save Birthday and Anniversary
	if( Birthday.HasData() )
		BuildField(data, offset, CFC_BIRTHDAY, Birthday.ToBBString());
	if( Anniversary.HasData() )
		BuildField(data, offset, CFC_ANNIVERSARY, Anniversary.ToBBString());

	// and finally save unknowns
	UnknownsType::const_iterator
		ub = Unknowns.begin(), ue = Unknowns.end();
	for( ; ub != ue; ub++ ) {
		BuildField(data, offset, *ub);
	}

	data.ReleaseBuffer(offset);
}

void Contact::Clear()
{
	RecType = GetDefaultRecType();
	RecordId = 0;

	EmailAddresses.clear();
	Phone.clear();

	Fax.clear();
	HomeFax.clear();
	WorkPhone.clear();
	HomePhone.clear();
	MobilePhone.clear();
	MobilePhone2.clear();
	Pager.clear();
	PIN.clear();
	Radio.clear();
	WorkPhone2.clear();
	HomePhone2.clear();
	OtherPhone.clear();
	FirstName.clear();
	LastName.clear();
	Company.clear();
	DefaultCommunicationsMethod.clear();
	JobTitle.clear();
	PublicKey.clear();
	URL.clear();
	Prefix.clear();
	Notes.clear();
	UserDefined1.clear();
	UserDefined2.clear();
	UserDefined3.clear();
	UserDefined4.clear();
	Image.clear();
	Nickname.clear();

	Birthday.Clear();
	Anniversary.Clear();

	WorkAddress.Clear();
	HomeAddress.Clear();

	Categories.clear();

 	GroupLinks.clear();
	Unknowns.clear();

	m_FirstNameSeen = false;
}

const FieldHandle<Contact>::ListT& Contact::GetFieldHandles()
{
	static FieldHandle<Contact>::ListT fhv;

	if( fhv.size() )
		return fhv;

#undef CONTAINER_OBJECT_NAME
#define CONTAINER_OBJECT_NAME fhv

#undef RECORD_CLASS_NAME
#define RECORD_CLASS_NAME Contact

	// first number is priority of fields... 0 being most critical fields
	FHP(RecType, "Record Type Code");
	FHP(RecordId, "Unique ID");
	FHP(EmailAddresses, "Email Addresses");

	FHP(FirstName, "First Name");
	FHP(LastName, "Last Name");
	FHL(Company, "Company", CFC_COMPANY, true, "o", 0);
	FHL(JobTitle, "Job Title", CFC_TITLE, true, "title", 0);
	FHD(Prefix, "Prefix", CFC_PREFIX, true);

	FHD(Nickname, "Nickname", CFC_NICKNAME, true);
	FHD(Phone, "Phone (deprecated)", CFC_PHONE, true);
	FHL(Fax, "Work Fax", CFC_FAX, true, "facsimileTelephoneNumber", 0);
	FHD(HomeFax, "Home Fax", CFC_HOME_FAX, true);
	FHL(WorkPhone, "Work Phone", CFC_WORK_PHONE, true,
		"telephoneNumber", 0);
	FHD(WorkPhone2, "Work Phone 2", CFC_WORK_PHONE_2, true);
	FHL(HomePhone, "Home Phone", CFC_HOME_PHONE, true, "homePhone", 0);
	FHD(HomePhone2, "Home Phone 2", CFC_HOME_PHONE_2, true);
	FHL(MobilePhone, "Mobile Phone", CFC_MOBILE_PHONE, true, "mobile", 0);
	FHD(MobilePhone2, "Mobile Phone 2", CFC_MOBILE_PHONE_2, true);
	FHD(OtherPhone, "Other Phone", CFC_OTHER_PHONE, true);
	FHL(Pager, "Pager", CFC_PAGER, true, "pager", 0);
	FHD(PIN, "PIN", CFC_PIN, true);
	FHD(Radio, "Radio", CFC_RADIO, true);
	FHD(DefaultCommunicationsMethod, "Default Communications Method",
		CFC_DEFAULT_COMM_METHOD, true);
	FHD(PublicKey, "Public Key", CFC_PUBLIC_KEY, false);
	FHD(URL, "URL", CFC_URL, true);
	FHD(Notes, "Notes", CFC_NOTES, true);
	FHD(UserDefined1, "User Defined Field 1", CFC_USER_DEFINED_1, true);
	FHD(UserDefined2, "User Defined Field 2", CFC_USER_DEFINED_2, true);
	FHD(UserDefined3, "User Defined Field 3", CFC_USER_DEFINED_3, true);
	FHD(UserDefined4, "User Defined Field 4", CFC_USER_DEFINED_4, true);
	FHD(Image, "Image", CFC_IMAGE, false);

	FHD(Birthday, "Birthday", CFC_BIRTHDAY, true);
	FHD(Anniversary, "Anniversary", CFC_ANNIVERSARY, true);

	FHC(WorkAddress, "Work Address");
	FHS(WorkAddress, Address1, "Work Address 1",
					CFC_ADDRESS1, true, 0, 0);
	FHS(WorkAddress, Address2, "Work Address 2",
					CFC_ADDRESS2, true, 0, 0);
	FHS(WorkAddress, Address3, "Work Address 3",
					CFC_ADDRESS3, true, 0, 0);
	FHS(WorkAddress, City, "Work City",
					CFC_CITY, true, "l", 0);
	FHS(WorkAddress, Province, "Work Province",
					CFC_PROVINCE, true, "st", 0);
	FHS(WorkAddress, PostalCode, "Work Postal Code",
					CFC_POSTAL_CODE, true, "postalCode", 0);
	FHS(WorkAddress, Country, "Work Country",
					CFC_COUNTRY, true, "c", "country");

	FHC(HomeAddress, "Home Address");
	FHS(HomeAddress, Address1, "Home Address 1",
					CFC_HOME_ADDRESS1, true, 0, 0);
	FHS(HomeAddress, Address2, "Home Address 2",
					CFC_HOME_ADDRESS2, true, 0, 0);
	FHS(HomeAddress, Address3, "Home Address 3",
					CFC_HOME_ADDRESS3, true, 0, 0);
	FHS(HomeAddress, City, "Home City",
					CFC_HOME_CITY, true, 0, 0);
	FHS(HomeAddress, Province, "Home Province",
					CFC_HOME_PROVINCE, true, 0, 0);
	FHS(HomeAddress, PostalCode, "Home Postal Code",
					CFC_HOME_POSTAL_CODE, true, 0, 0);
	FHS(HomeAddress, Country, "Home Country",
					CFC_HOME_COUNTRY, true, 0, 0);

	FHP(Categories, "Categories");
//	FHP(GroupLinks, "Group Links");
	FHP(Unknowns, "Unknown Fields");

	return fhv;
}

std::string Contact::GetDescription() const
{
	string desc = GetFullName();
	if( desc.size() == 0 && Company.size() )
		return Company;
	return desc;
}

//
// GetFullName
//
/// Helper function that returns a formatted full name
///
std::string Contact::GetFullName() const
{
	std::string Full = FirstName;
	if( Full.size() && LastName.size() )
		Full += " ";
	Full += LastName;
	return Full;
}

//
// GetEmail
//
/// Helper function that always returns a valid string.  The string
/// may be empty if there is no address at the specified index.
///
const std::string& Contact::GetEmail(unsigned int index) const
{
	static const std::string blank;
	if( index < EmailAddresses.size() )
		return EmailAddresses[index];
	return blank;
}

void Contact::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os.setf(ios::left);
	os.fill(' ');

	os << "Contact: 0x" << setbase(16) << GetID()
		<< " (" << (unsigned int)RecType << ")\n";

	// special fields not in type table
	os << "    " << setw(20) << "FirstName";
	os << ": " << FirstName << "\n";
	os << "    " << setw(20) << "LastName";
	os << ": " << LastName << "\n";

	// cycle through email addresses
	EmailList::const_iterator eai = EmailAddresses.begin();
	for( ; eai != EmailAddresses.end(); ++eai ) {
		if( eai->size() ) {
			os << "    Email               : " << *eai << "\n";
		}
	}

	// cycle through the type table
	for(	FieldLink<Contact> *b = ContactFieldLinks;
		b->type != CFC_INVALID_FIELD;
		b++ )
	{
		// special case: don't dump the raw image data, but
		// leave that for a special hex dump
		if( b->type == CFC_IMAGE )
			continue;

		const std::string *pField = 0;
		if( b->strMember ) {
			pField = &(this->*(b->strMember));
		}
		else if( b->postMember && b->postField ) {
			pField = &((this->*(b->postMember)).*(b->postField));
		}

		// print only fields with data
		if( pField && pField->size() ) {
			os << "    " << setw(20) << b->name;
			os << ": " << Cr2LfWrapper(*pField) << "\n";
		}
	}

	if( Categories.size() ) {
		string display;
		Categories.CategoryList2Str(display);
		os << "    Categories          : " << display << "\n";
	}

	// print Birthday and Anniversary
	if( Birthday.HasData() ) {
		os << "    Birthday            : " << Birthday << "\n";
	}
	if( Anniversary.HasData() ) {
		os << "    Anniversary         : " << Anniversary << "\n";
	}

	// print any group links
	GroupLinksType::const_iterator
		gb = GroupLinks.begin(), ge = GroupLinks.end();
	if( gb != ge )
		os << "    GroupLinks:\n";
	for( ; gb != ge; gb++ ) {
		os << "        ID: 0x" << setbase(16) << gb->Link << "\n";
	}

	// print Image in hex dump format, if available
	if( Image.size() ) {
		Data image(Image.data(), Image.size());
		os << "    Photo image:\n";
		os << image << "\n";
	}

	// and finally print unknowns
	os << Unknowns;
}

bool Contact::operator<(const Contact &other) const
{
	// old sorting mechanism, to put group links at the bottom
	//return GroupLinks.size() == 0 && other.GroupLinks.size() > 0;
	// testing - put group links at the top
	//return GroupLinks.size() > 0 && other.GroupLinks.size() == 0;

	// usually one of these fields is filled in, so compare
	// them all in a ( LastName + FirstName + Company ) key style
	int cmp = LastName.compare(other.LastName);
	if( cmp == 0 )
		cmp = FirstName.compare(other.FirstName);
	if( cmp == 0 )
		cmp = Company.compare(other.Company);
	return cmp < 0;
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

std::string Contact::Email2CommaString(const EmailList &list)
{
	ostringstream oss;
	for( EmailList::const_iterator i = list.begin(); i!=list.end(); ++i ) {
		if( i != list.begin() )
			oss << ", ";
		oss << *i;
	}
	return oss.str();
}

/// Replaces the EmailAddresses list with the parsed results of
/// list.  If list is empty, then EmailAddresses will also be empty.
/// Note that incoming addresses need to be in simple format, not
/// complex formats like "Name <user@example.com>" but just
/// "user@example.com".  This is a device limitation.
///
/// Any complex email addresses found in the list will be dropped,
/// with a message sent to the debug output stream.
void Contact::CommaString2Email(const std::string &list, EmailList &result)
{
	// start fresh
	result.clear();

	// parse the comma separated list
	istringstream iss(list);
	string address;

	while( iss >> ws && getline(iss, address, ',') ) {
		// trim any trailing whitespace in the address
		Inplace::rtrim(address);

		// is this a complex address?  like:
		// Chris Frey <cdfrey@foursquare.net>
		// The device only accepts the plain
		// "cdfrey@foursquare.net" part here
		if( address.rfind('>') != string::npos ) {
			dout("Error: Cannot convert complex name+address to a simple contact email address, skipping: " << address);
			continue;
		}

		// add to list if anything left
		if( address.size() ) {
			result.push_back(address);
		}
	}
}

} // namespace Barry

