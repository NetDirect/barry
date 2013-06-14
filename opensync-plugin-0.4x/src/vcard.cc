///
/// \file	vcard.cc
///		Conversion routines for vcards
///

/*
    Copyright (C) 2006-2013, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <stdint.h>
#include <glib.h>
#include <string.h>
#include <sstream>
#include <ctype.h>
#include "i18n.h"

using namespace Barry::Sync;

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
	catch( Barry::ConvertError &ce ) {
		trace.logf(_("ERROR: vcard:Barry::ConvertError exception: %s"), ce.what());
		m_last_errmsg = ce.what();
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
	catch( Barry::ConvertError &ce ) {
		trace.logf(_("ERROR: vcard:Barry::ConvertError exception: %s"), ce.what());
		m_last_errmsg = ce.what();
	}
}

// Barry builder operator
bool VCardConverter::operator()(Barry::Contact &rec, Barry::Builder &)
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
	env->GetDesktop()->GetRecord(dbId, index, parser);
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
			trace.log(_("Can't use recommended recordId, generating new one."));
			newRecordId = env->m_ContactsSync.m_Table.MakeNewRecordId();
		}
	}
	else {
		newRecordId = env->m_ContactsSync.m_Table.StateMap[StateIndex].RecordId;
	}
	trace.logf("newRecordId: %u", newRecordId);

	VCardConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << _("unable to parse change data for new RecordId: ")
		    << newRecordId
		    << " (" << convert.GetLastError() << ") "
		    << _("data: ") << data;
		errmsg = oss.str();
		trace.log(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Contact, VCardConverter> builder(convert);

	if( add ) {
		trace.log(_("adding record"));
		env->GetDesktop()->AddRecord(dbId, builder);
	}
	else {
		trace.log(_("setting record"));
		env->GetDesktop()->SetRecord(dbId, StateIndex, builder);
		trace.log(_("clearing dirty flag"));
		env->GetDesktop()->ClearDirty(dbId, StateIndex);
	}

	return true;
}

