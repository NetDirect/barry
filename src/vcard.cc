///
/// \file	vcard.cc
///		Conversion routines for vcards
///

/*
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "vcard.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sstream>

namespace Barry { namespace Sync {

//////////////////////////////////////////////////////////////////////////////
// Utility functions

namespace {

	void ToLower(std::string &str)
	{
		size_t i = 0;
		while( i < str.size() ) {
			str[i] = tolower(str[i]);
			i++;
		}
	}

}

//////////////////////////////////////////////////////////////////////////////
// vCard

vCard::vCard()
	: m_gCardData(0)
{
}

vCard::~vCard()
{
	if( m_gCardData ) {
		g_free(m_gCardData);
	}
}

void vCard::AddAddress(const char *rfc_type, const Barry::PostalAddress &address)
{
	// add label first
	vAttrPtr label = NewAttr("LABEL");
	AddParam(label, "TYPE", rfc_type);
	AddValue(label, address.GetLabel().c_str());
	AddAttr(label);

	// add breakout address form
	vAttrPtr adr = NewAttr("ADR");			// RFC 2426, 3.2.1
	AddParam(adr, "TYPE", rfc_type);
	AddValue(adr, address.Address3.c_str());	// PO Box
	AddValue(adr, address.Address2.c_str());	// Extended address
	AddValue(adr, address.Address1.c_str());	// Street address
	AddValue(adr, address.City.c_str());		// Locality (city)
	AddValue(adr, address.Province.c_str());	// Region (province)
	AddValue(adr, address.PostalCode.c_str());	// Postal code
	AddValue(adr, address.Country.c_str());		// Country name
	AddAttr(adr);
}

/// Add phone conditionally, only if phone has data in it.  This version
/// does not add a TYPE parameter to the item.
void vCard::AddPhoneCond(const std::string &phone)
{
	if( phone.size() ) {
		vAttrPtr tel = NewAttr("TEL", phone.c_str());
		AddAttr(tel);
	}
}

/// Add phone conditionally, only if phone has data in it
void vCard::AddPhoneCond(const char *rfc_type, const std::string &phone)
{
	if( phone.size() ) {
		vAttrPtr tel = NewAttr("TEL", phone.c_str());
		AddParam(tel, "TYPE", rfc_type);
		AddAttr(tel);
	}
}

void vCard::ParseAddress(vAttr &adr, Barry::PostalAddress &address)
{
	// RFC 2426, 3.2.1
	address.Address3 = adr.GetValue(0);		// PO Box
	address.Address2 = adr.GetValue(1);		// Extended address
	address.Address1 = adr.GetValue(2);		// Street address
	address.City = adr.GetValue(3);			// Locality (city)
	address.Province = adr.GetValue(4);		// Region (province)
	address.PostalCode = adr.GetValue(5);		// Postal code
	address.Country = adr.GetValue(6);		// Country name
}

void vCard::ParseCategories(vAttr &cat, Barry::CategoryList &cats)
{
	int i = 0;
	std::string value = cat.GetValue(i);
	while( value.size() ) {
		cats.push_back(value);
		i++;
		value = cat.GetValue(i);
	}
}



// Main conversion routine for converting from Barry::Contact to
// a vCard string of data.
const std::string& vCard::ToVCard(const Barry::Contact &con)
{
//	Trace trace("vCard::ToVCard");
	std::ostringstream oss;
	con.Dump(oss);
//	trace.logf("ToVCard, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryContact = con;

	//
	// begin building vCard data
	//

	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Contact Record//EN"));

	std::string fullname = con.GetFullName();
	if( fullname.size() ) {
		AddAttr(NewAttr("FN", fullname.c_str()));
	}
	else {
		//
		// RFC 2426, 3.1.1 states that FN MUST be present in the
		// vcard object.  Unfortunately, the Blackberry doesn't
		// require a name, only a name or company name.
		//
		// In this case we do nothing, and generate an invalid
		// vcard, since if we try to fix our output here, we'll
		// likely end up with duplicated company names in the
		// Blackberry record after a few syncs.
		//
	}

	if( con.FirstName.size() || con.LastName.size() ) {
		vAttrPtr name = NewAttr("N");		// RFC 2426, 3.1.2
		AddValue(name, con.LastName.c_str());	// Family Name
		AddValue(name, con.FirstName.c_str());	// Given Name
		AddValue(name, "");			// Additional Names
		AddValue(name, con.Prefix.c_str());	// Honorific Prefixes
		AddValue(name, "");			// Honorific Suffixes
		AddAttr(name);
	}

	if( con.Nickname.size() )
		AddAttr(NewAttr("NICKNAME", con.Nickname.c_str()));

	if( con.WorkAddress.HasData() )
		AddAddress("work", con.WorkAddress);
	if( con.HomeAddress.HasData() )
		AddAddress("home", con.HomeAddress);

	// add all applicable phone numbers... there can be multiple
	// TEL fields, even with the same TYPE value... therefore, the
	// second TEL field with a TYPE=work, will be stored in WorkPhone2
	AddPhoneCond("voice,pref", con.Phone);
	AddPhoneCond("fax", con.Fax);
	AddPhoneCond("voice,work", con.WorkPhone);
	AddPhoneCond("voice,work", con.WorkPhone2);
	AddPhoneCond("voice,home", con.HomePhone);
	AddPhoneCond("voice,home", con.HomePhone2);
	AddPhoneCond("msg,cell", con.MobilePhone);
	AddPhoneCond("msg,pager", con.Pager);
	AddPhoneCond("voice", con.OtherPhone);

	// add all email addresses, marking first one as "pref"
	Barry::Contact::EmailList::const_iterator eai = con.EmailAddresses.begin();
	for( unsigned int i = 0; eai != con.EmailAddresses.end(); ++eai, ++i ) {
		const std::string& e = con.GetEmail(i);
		if( e.size() ) {
			vAttrPtr email = NewAttr("EMAIL", e.c_str());
			if( i == 0 ) {
				AddParam(email, "TYPE", "internet,pref");
			}
			else {
				AddParam(email, "TYPE", "internet");
			}
			AddAttr(email);
		}
	}

	if( con.JobTitle.size() ) {
		AddAttr(NewAttr("TITLE", con.JobTitle.c_str()));
		AddAttr(NewAttr("ROLE", con.JobTitle.c_str()));
	}

	if( con.Company.size() ) {
		// RFC 2426, 3.5.5
		vAttrPtr org = NewAttr("ORG", con.Company.c_str()); // Organization name
		AddValue(org, "");			// Division name
		AddAttr(org);
	}

	if( con.Birthday.HasData() )
		AddAttr(NewAttr("BDAY", con.Birthday.ToYYYYMMDD().c_str()));

	if( con.Notes.size() )
		AddAttr(NewAttr("NOTE", con.Notes.c_str()));
	if( con.URL.size() )
		AddAttr(NewAttr("URL", con.URL.c_str()));
	if( con.Categories.size() )
		AddCategories(con.Categories);

	// Image / Photo
	if (con.Image.size()) {
		vAttrPtr photo = NewAttr("PHOTO");
		AddEncodedValue(photo, VF_ENCODING_BASE64, con.Image.c_str(), con.Image.size());
		AddParam(photo, "ENCODING", "BASE64");
		AddAttr(photo);
	}

	// generate the raw VCARD data
	m_gCardData = b_vformat_to_string(Format(), VFORMAT_CARD_30);
	m_vCardData = m_gCardData;

//	trace.logf("ToVCard, resulting vcard data: %s", m_vCardData.c_str());
	return m_vCardData;
}

//
// NOTE:
//	Treat the following pairs of variables like
//	sliding buffers, where higher priority values
//	can push existings values from 1 to 2, or from
//	2 to oblivion:
//
//		HomePhone + HomePhone2
//		WorkPhone + WorkPhone2
//		Phone + OtherPhone
//
//
// class SlidingPair
//
// This class handles the sliding pair logic for a pair of std::strings.
//
class SlidingPair
{
	std::string &m_first;
	std::string &m_second;
	int m_evolutionSlot1, m_evolutionSlot2;
public:
	static const int DefaultSlot = 99;
	SlidingPair(std::string &first, std::string &second)
		: m_first(first)
		, m_second(second)
		, m_evolutionSlot1(DefaultSlot)
		, m_evolutionSlot2(DefaultSlot)
	{
	}

	bool assign(const std::string &value, const char *type_str, int evolutionSlot)
	{
		bool used = false;

		if( strstr(type_str, "pref") || evolutionSlot < m_evolutionSlot1 ) {
			m_second = m_first;
			m_evolutionSlot2 = m_evolutionSlot1;

			m_first = value;
			m_evolutionSlot1 = evolutionSlot;

			used = true;
		}
		else if( evolutionSlot < m_evolutionSlot2 ) {
			m_second = value;
			m_evolutionSlot2 = evolutionSlot;
			used = true;
		}
		else if( m_first.size() == 0 ) {
			m_first = value;
			m_evolutionSlot1 = evolutionSlot;
			used = true;
		}
		else if( m_second.size() == 0 ) {
			m_second = value;
			m_evolutionSlot2 = evolutionSlot;
			used = true;
		}

		return used;
	}
};


// Main conversion routine for converting from vCard data string
// to a Barry::Contact object.
const Barry::Contact& vCard::ToBarry(const char *vcard, uint32_t RecordId)
{
	using namespace std;

//	Trace trace("vCard::ToBarry");
//	trace.logf("ToBarry, working on vcard data: %s", vcard);

	// start fresh
	Clear();

	// store the vCard raw data
	m_vCardData = vcard;

	// create format parser structures
	SetFormat( b_vformat_new_from_string(vcard) );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");


	//
	// Parse the vcard data
	//

	Barry::Contact &con = m_BarryContact;
	con.SetIds(Barry::Contact::GetDefaultRecType(), RecordId);

	vAttr name = GetAttrObj("N");
	if( name.Get() ) {
							// RFC 2426, 3.1.2
		con.LastName = name.GetValue(0);	// Family Name
		con.FirstName = name.GetValue(1);	// Given Name
		con.Prefix = name.GetValue(3);		// Honorific Prefixes
	}

	con.Nickname = GetAttr("NICKNAME");

	vAttr adr = GetAttrObj("ADR");
	for( int i = 0; adr.Get(); adr = GetAttrObj("ADR", ++i) )
	{
		std::string type = adr.GetAllParams("TYPE");
		ToLower(type);

		// do not use "else" here, since TYPE can have multiple keys
		if( strstr(type.c_str(), "work") )
			ParseAddress(adr, con.WorkAddress);
		if( strstr(type.c_str(), "home") )
			ParseAddress(adr, con.HomeAddress);
	}


	//
	// NOTE:
	//	Treat the following pairs of variables like
	//	sliding buffers, where higher priority values
	//	can push existings values from 1 to 2, or from
	//	2 to oblivion:
	//
	//		HomePhone + HomePhone2
	//		WorkPhone + WorkPhone2
	//		Phone + OtherPhone
	//
	SlidingPair HomePair(con.HomePhone, con.HomePhone2);
	SlidingPair WorkPair(con.WorkPhone, con.WorkPhone2);
	SlidingPair OtherPair(con.Phone, con.OtherPhone);

	// add all applicable phone numbers... there can be multiple
	// TEL fields, even with the same TYPE value... therefore, the
	// second TEL field with a TYPE=work, will be stored in WorkPhone2
	vAttr tel = GetAttrObj("TEL");
	for( int i = 0; tel.Get(); tel = GetAttrObj("TEL", ++i) )
	{
		// grab all parameter values for this param name
		std::string stype = tel.GetAllParams("TYPE");

		// grab evolution-specific parameter... evolution is too
		// lazy to sort its VCARD output, but instead it does
		// its own non-standard tagging... so we try to
		// accommodate it, so Work and Home phone numbers keep
		// their order if possible
		int evolutionSlot = atoi(tel.GetAllParams("X-EVOLUTION-UI-SLOT").c_str());
		if( evolutionSlot == 0 )
			evolutionSlot = SlidingPair::DefaultSlot;

		// turn to lower case for comparison
		// FIXME - is this i18n safe?
		ToLower(stype);

		// state
		const char *type = stype.c_str();
		bool used = false;

		// Check for possible TYPE conflicts:
		//    pager can coexist with cell/pcs/car
		//    fax conflicts with cell/pcs/car
		//    fax conflicts with pager
		bool mobile_type = strstr(type, "cell") ||
			strstr(type, "pcs") ||
			strstr(type, "car");
		bool fax_type = strstr(type, "fax");
		bool pager_type = strstr(type, "pager");
		if( fax_type && (mobile_type || pager_type) ) {
			// conflict found, log and skip
//			trace.logf("ToBarry: skipping phone number due to TYPE conflict: fax cannot coexist with %s: %s",
//				mobile_type ? "cell/pcs/car" : "pager",
//				type);
			continue;
		}

		// If phone number has the "pref" flag
		if( strstr(type, "pref") ) {
			// Always use cell phone if the "pref" flag is set
			if( strstr(type, "cell") ) {
				used = OtherPair.assign(tel.GetValue(), type, evolutionSlot);
			}
			// Otherwise, the phone has to be "voice" type
			else if( strstr(type, "voice") && con.Phone.size() == 0 ) {
				used = OtherPair.assign(tel.GetValue(), type, evolutionSlot);
			}
		}

		// For each known phone type
		// Fax :
		if( strstr(type, "fax") && (strstr(type, "pref") || con.Fax.size() == 0) ) {
			con.Fax = tel.GetValue();
			used = true;
		}
		// Mobile phone :
		else if( mobile_type && (strstr(type, "pref") || con.MobilePhone.size() == 0) ) {
			con.MobilePhone = tel.GetValue();
			used = true;
		}
		// Pager :
		else if( strstr(type, "pager") && (strstr(type, "pref") || con.Pager.size() == 0) ) {
			con.Pager = tel.GetValue();
			used = true;
		}
		// Check for any TEL-ignore types, and use other phone field if possible
		// bbs/video/modem   entire TEL ignored by Barry
		// isdn              entire TEL ignored by Barry
		else if( strstr(type, "bbs") || strstr(type, "video") || strstr(type, "modem") ) {
		}
		else if( strstr(type, "isdn") ) {
		}
		// Voice telephone :
		else {
			if( strstr(type, "work") ) {
				used = WorkPair.assign(tel.GetValue(), type, evolutionSlot);
			}

			if( strstr(type, "home") ) {
				used = HomePair.assign(tel.GetValue(), type, evolutionSlot);
			}
		}

		// if this value has not been claimed by any of the
		// cases above, claim it now as "OtherPhone"
		if( !used && con.OtherPhone.size() == 0 ) {
			OtherPair.assign(tel.GetValue(), type, evolutionSlot);
		}
	}

	// scan for all email addresses... append addresses to the
	// list by default, but prepend if its type is set to "pref"
	// i.e. we want the preferred email address first
	vAttr email = GetAttrObj("EMAIL");
	for( int i = 0; email.Get(); email = GetAttrObj("EMAIL", ++i) )
	{
		std::string type = email.GetAllParams("TYPE");
		ToLower(type);

		bool of_interest = (i == 0 || strstr(type.c_str(), "pref"));
		bool x400 = strstr(type.c_str(), "x400");

		if( of_interest && !x400 ) {
			con.EmailAddresses.insert(con.EmailAddresses.begin(), email.GetValue());
		}
		else {
			con.EmailAddresses.push_back( email.GetValue() );
		}
	}

	// figure out which company title we want to keep...
	// favour the TITLE field, but if it's empty, use ROLE
	con.JobTitle = GetAttr("TITLE");
	if( !con.JobTitle.size() )
		con.JobTitle = GetAttr("ROLE");

	con.Company = GetAttr("ORG");
	con.Notes = GetAttr("NOTE");
	con.URL = GetAttr("URL");
	if( GetAttr("BDAY").size() && !con.Birthday.FromYYYYMMDD( GetAttr("BDAY") ) )
		throw ConvertError("Unable to parse BDAY field");

	// Photo vCard ?
	vAttr photo = GetAttrObj("PHOTO");
	if (photo.Get()) {
		std::string sencoding = photo.GetAllParams("ENCODING");

		ToLower(sencoding);

		const char *encoding = sencoding.c_str();

		if (strstr(encoding, "quoted-printable")) {
			photo.Get()->encoding = VF_ENCODING_QP;

			con.Image = photo.GetDecodedValue();
		}
		else if (strstr(encoding, "b")) {
			photo.Get()->encoding = VF_ENCODING_BASE64;

			con.Image = photo.GetDecodedValue();
		}
		// Else
		// We ignore the photo, I don't know decoded !
	}

	vAttr cat = GetAttrObj("CATEGORIES");
	if( cat.Get() )
		ParseCategories(cat, con.Categories);

	// Last sanity check: Blackberry requires that at least
	// name or Company has data.
	if( !con.GetFullName().size() && !con.Company.size() )
		throw ConvertError("FN and ORG fields both blank in VCARD data");

	return m_BarryContact;
}

// Transfers ownership of m_gCardData to the caller.
char* vCard::ExtractVCard()
{
	char *ret = m_gCardData;
	m_gCardData = 0;
	return ret;
}

void vCard::Clear()
{
	vBase::Clear();
	m_vCardData.clear();
	m_BarryContact.Clear();

	if( m_gCardData ) {
		g_free(m_gCardData);
		m_gCardData = 0;
	}
}

}} // namespace Barry::Sync

