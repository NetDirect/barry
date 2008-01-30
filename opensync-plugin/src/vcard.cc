///
/// \file	vcard.cc
///		Conversion routines for vcards
///

/*
    Copyright (C) 2006-2008, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "environment.h"
#include "trace.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include <stdint.h>
#include <glib.h>
#include <sstream>
#include <ctype.h>


//////////////////////////////////////////////////////////////////////////////
// Utility functions

void ToLower(std::string &str)
{
	size_t i = 0;
	while( i < str.size() ) {
		str[i] = tolower(str[i]);
		i++;
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

void vCard::AddCategories(const Barry::CategoryList &categories)
{
	if( !categories.size() )
		return;

	vAttrPtr cat = NewAttr("CATEGORIES");		// RFC 2426, 3.6.1
	Barry::CategoryList::const_iterator i = categories.begin();
	for( ; i < categories.end(); ++i ) {
		AddValue(cat, i->c_str());
	}
	AddAttr(cat);
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
	Trace trace("vCard::ToVCard");
	std::ostringstream oss;
	con.Dump(oss);
	trace.logf("ToVCard, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryContact = con;

	//
	// begin building vCard data
	//

	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Contact Record//EN"));

	std::string fullname = con.GetFullName();
	if( fullname.size() )
		AddAttr(NewAttr("FN", fullname.c_str()));

	if( con.FirstName.size() || con.LastName.size() ) {
		vAttrPtr name = NewAttr("N");		// RFC 2426, 3.1.2
		AddValue(name, con.LastName.c_str());	// Family Name
		AddValue(name, con.FirstName.c_str());	// Given Name
		AddValue(name, "");			// Additional Names
		AddValue(name, con.Prefix.c_str());	// Honorific Prefixes
		AddValue(name, "");			// Honorific Suffixes
		AddAttr(name);
	}

	if( con.WorkAddress.HasData() )
		AddAddress("work", con.WorkAddress);
	if( con.HomeAddress.HasData() )
		AddAddress("home", con.HomeAddress);

	// add all applicable phone numbers... can't add the WorkPhone2
	// since VCARD30 TEL fields only take one number, as far as I know
	AddPhoneCond("pref", con.Phone);
	AddPhoneCond("fax", con.Fax);
	AddPhoneCond("work", con.WorkPhone);
	AddPhoneCond("home", con.HomePhone);
	AddPhoneCond("cell", con.MobilePhone);
	AddPhoneCond("msg", con.Pager);

	if( con.Email.size() ) {
		vAttrPtr email = NewAttr("EMAIL", con.Email.c_str());
		AddParam(email, "TYPE", "internet");
		AddAttr(email);
	}

	if( con.JobTitle.size() ) {
		AddAttr(NewAttr("TITLE", con.JobTitle.c_str()));
		AddAttr(NewAttr("ROLE", con.JobTitle.c_str()));
	}

	// Image not supported, since vformat routines probably don't
	// support binary VCARD fields....

	if( con.Company.size() ) {
		// RFC 2426, 3.5.5
		vAttrPtr org = NewAttr("ORG", con.Company.c_str()); // Organization name
		AddValue(org, "");			// Division name
		AddAttr(org);
	}

	if( con.Notes.size() )
		AddAttr(NewAttr("NOTE", con.Notes.c_str()));
	if( con.URL.size() )
		AddAttr(NewAttr("URL", con.URL.c_str()));
	if( con.Categories.size() )
		AddCategories(con.Categories);

	// generate the raw VCARD data
	m_gCardData = vformat_to_string(Format(), VFORMAT_CARD_30);
	m_vCardData = m_gCardData;

	trace.logf("ToVCard, resulting vcard data: %s", m_vCardData.c_str());
	return m_vCardData;
}

// Main conversion routine for converting from vCard data string
// to a Barry::Contact object.
const Barry::Contact& vCard::ToBarry(const char *vcard, uint32_t RecordId)
{
	using namespace std;

	Trace trace("vCard::ToBarry");
	trace.logf("ToBarry, working on vcard data: %s", vcard);

	// start fresh
	Clear();

	// store the vCard raw data
	m_vCardData = vcard;

	// create format parser structures
	SetFormat( vformat_new_from_string(vcard) );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");


	//
	// Parse the vcard data
	//

	Barry::Contact &con = m_BarryContact;
	con.SetIds(Barry::Contact::GetDefaultRecType(), RecordId);

	vAttr name = GetAttrObj("N");
	if( !name.Get() )
		throw ConvertError("no FN field in VCARD data");
						// RFC 2426, 3.1.2
	con.LastName = name.GetValue(0);	// Family Name
	con.FirstName = name.GetValue(1);	// Given Name
	con.Prefix = name.GetValue(3);		// Honorific Prefixes

	vAttr adr = GetAttrObj("ADR");
	for( int i = 0; adr.Get(); adr = GetAttrObj("ADR", ++i) )
	{
		std::string type = adr.GetParam("TYPE");
		ToLower(type);

		// do not use "else" here, since TYPE can have multiple keys
		if( strstr(type.c_str(), "work") )
			ParseAddress(adr, con.WorkAddress);
		if( strstr(type.c_str(), "home") )
			ParseAddress(adr, con.HomeAddress);
	}


	// add all applicable phone numbers... can't add the WorkPhone2
	// since VCARD30 TEL fields only take one number, as far as I know
	vAttr tel = GetAttrObj("TEL");
	for( int i = 0; tel.Get(); tel = GetAttrObj("TEL", ++i) )
	{
		std::string stype = tel.GetParam("TYPE");
		ToLower(stype);
		const char *type = stype.c_str();

		// do not use "else" here, since TYPE can have multiple keys
		if( strstr(type, "pref") )
			con.Phone = tel.GetValue();
		if( strstr(type, "fax") )
			con.Fax = tel.GetValue();
		if( strstr(type, "work") )
			con.WorkPhone = tel.GetValue();
		if( strstr(type, "home") )
			con.HomePhone = tel.GetValue();
		if( strstr(type, "cell") )
			con.MobilePhone = tel.GetValue();
		if( strstr(type, "msg") )
			con.Pager = tel.GetValue();
	}

	// scan for all email addresses... save the first one found
	// by default, then overwrite it with any following email
	// address if its type is set to "pref"... i.e. we want
	// the preferred email address here.
	vAttr email = GetAttrObj("EMAIL");
	for( int i = 0; email.Get(); email = GetAttrObj("EMAIL", ++i) )
	{
		std::string type = email.GetParam("TYPE");
		ToLower(type);

		bool of_interest = (i == 0 || strstr(type.c_str(), "pref"));
		bool x400 = strstr(type.c_str(), "x400");

		if( of_interest && !x400 ) {
			con.Email = GetAttr("EMAIL");
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

	vAttr cat = GetAttrObj("CATEGORIES");
	if( cat.Get() )
		ParseCategories(cat, con.Categories);

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



//////////////////////////////////////////////////////////////////////////////
//

VCardConverter::VCardConverter()
	: m_Data(0)
{
}

VCardConverter::VCardConverter(uint32_t newRecordId)
	: m_Data(0),
	m_RecordId(newRecordId)
{
}

VCardConverter::~VCardConverter()
{
	if( m_Data )
		g_free(m_Data);
}

// Transfers ownership of m_Data to the caller
char* VCardConverter::ExtractData()
{
	Trace trace("VCardConverter::ExtractData");
	char *ret = m_Data;
	m_Data = 0;
	return ret;
}

bool VCardConverter::ParseData(const char *data)
{
	Trace trace("VCardConverter::ParseData");

	try {

		vCard vcard;
		m_Contact = vcard.ToBarry(data, m_RecordId);

	}
	catch( vCard::ConvertError &ce ) {
		trace.logf("ERROR: vCard::ConvertError exception: %s", ce.what());
		return false;
	}

	return true;
}

// Barry storage operator
void VCardConverter::operator()(const Barry::Contact &rec)
{
	Trace trace("VCardConverter::operator()");

	// Delete data if some already exists
	if( m_Data ) {
		g_free(m_Data);
		m_Data = 0;
	}

	try {

		vCard vcard;
		vcard.ToVCard(rec);
		m_Data = vcard.ExtractVCard();

	}
	catch( vCard::ConvertError &ce ) {
		trace.logf("ERROR: vCard::ConvertError exception: %s", ce.what());
	}
}

// Barry builder operator
bool VCardConverter::operator()(Barry::Contact &rec, unsigned int dbId)
{
	Trace trace("VCardConverter::builder operator()");

	rec = m_Contact;
	return true;
}

// Handles calling of the Barry::Controller to fetch a specific
// record, indicated by index (into the RecordStateTable).
// Returns a g_malloc'd string of data containing the vcard30
// data.  It is the responsibility of the caller to free it.
// This is intended to be passed into the GetChanges() function.
char* VCardConverter::GetRecordData(BarryEnvironment *env, unsigned int dbId,
				    Barry::RecordStateTable::IndexType index)
{
	Trace trace("VCardConverter::GetRecordData()");

	using namespace Barry;

	VCardConverter contact2vcard;
	RecordParser<Contact, VCardConverter> parser(contact2vcard);
	env->m_pDesktop->GetRecord(dbId, index, parser);
	return contact2vcard.ExtractData();
}

bool VCardConverter::CommitRecordData(BarryEnvironment *env, unsigned int dbId,
	Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
	const char *data, bool add, std::string &errmsg)
{
	Trace trace("VCardConverter::CommitRecordData()");

	uint32_t newRecordId;
	if( add ) {
		// use given id if possible
		if( recordId && !env->m_ContactsSync.m_Table.GetIndex(recordId) ) {
			// recordId is unique and non-zero
			newRecordId = recordId;
		}
		else {
			trace.log("Can't use recommended recordId, generating new one.");
			newRecordId = env->m_ContactsSync.m_Table.MakeNewRecordId();
		}
	}
	else {
		newRecordId = env->m_ContactsSync.m_Table.StateMap[StateIndex].RecordId;
	}
	trace.logf("newRecordId: %lu", newRecordId);

	VCardConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << "unable to parse change data for new RecordId: "
		    << newRecordId << " data: " << data;
		errmsg = oss.str();
		trace.logf(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Contact, VCardConverter> builder(convert);

	if( add ) {
		trace.log("adding record");
		env->m_pDesktop->AddRecord(dbId, builder);
	}
	else {
		trace.log("setting record");
		env->m_pDesktop->SetRecord(dbId, StateIndex, builder);
		trace.log("clearing dirty flag");
		env->m_pDesktop->ClearDirty(dbId, StateIndex);
	}

	return true;
}

