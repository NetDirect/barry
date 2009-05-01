//
// \file	vjournal.cc
//		Conversion routines for vjournals (VCALENDAR, etc)
//

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2006-2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <opensync/opensync.h>
#include <opensync/opensync-time.h>

#include "vjournal.h"
#include "environment.h"
#include "trace.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <sstream>


//////////////////////////////////////////////////////////////////////////////
// vJournal

vJournal::vJournal()
	: m_gJournalData(0)
{
}

vJournal::~vJournal()
{
	if( m_gJournalData ) {
		g_free(m_gJournalData);
	}
}

bool vJournal::HasMultipleVJournals() const
{
	int count = 0;
	b_VFormat *format = const_cast<b_VFormat*>(Format());
	GList *attrs = format ? b_vformat_get_attributes(format) : 0;
	for( ; attrs; attrs = attrs->next ) {
		b_VFormatAttribute *attr = (b_VFormatAttribute*) attrs->data;
		if( strcasecmp(b_vformat_attribute_get_name(attr), "BEGIN") == 0 &&
		    strcasecmp(b_vformat_attribute_get_nth_value(attr, 0), "VJOURNAL") == 0 )
		{
			count++;
		}
	}
	return count > 1;
}


// Main conversion routine for converting from Barry::Memo to
// a vJournal string of data.
const std::string& vJournal::ToMemo(const Barry::Memo &memo)
{
	Trace trace("vCalendar::ToMemo");
	std::ostringstream oss;
	memo.Dump(oss);
	trace.logf("ToMemo, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryMemo = memo;

	// begin building vCalendar data
	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Memo Record//EN"));
	AddAttr(NewAttr("BEGIN", "VJOURNAL"));
	AddAttr(NewAttr("SEQUENCE", "0"));
	AddAttr(NewAttr("SUMMARY", memo.Title.c_str()));
	AddAttr(NewAttr("DESCRIPTION", memo.Body.c_str()));

	// FIXME - add a truly globally unique "UID" string?

	AddAttr(NewAttr("END", "VJOURNAL"));

	// generate the raw VJOURNAL data
	m_gJournalData = b_vformat_to_string(Format(), VFORMAT_NOTE);
	m_vJournalData = m_gJournalData;

	trace.logf("ToVCal, resulting vmemo data: %s", m_vJournalData.c_str());
	return m_vJournalData;
}

// Main conversion routine for converting from vJournal data string
// to a Barry::Memo object.
const Barry::Memo& vJournal::ToBarry(const char *vjournal, uint32_t RecordId)
{
	using namespace std;

	Trace trace("vJournal::ToBarry");
	trace.logf("ToBarry, working on vmemo data: %s", vjournal);

	// we only handle vJournal data with one vmemo block
	if( HasMultipleVJournals() )
		throw ConvertError("vCalendar data contains more than one VJOURNAL block, unsupported");

	// start fresh
	Clear();

	// store the vJournal raw data
	m_vJournalData = vjournal;

	// create format parser structures
	SetFormat( b_vformat_new_from_string(vjournal) );
	if( !Format() )
		throw ConvertError("resource error allocating vjournal");

	string title = GetAttr("SUMMARY", "/vjournal");
	trace.logf("SUMMARY attr retrieved: %s", title.c_str());
	if( title.size() == 0 ) {
		title = "<blank subject>";
		trace.logf("ERROR: bad data, blank SUMMARY: %s", vjournal);
	}

	string body = GetAttr("DESCRIPTION", "/vjournal");
	trace.logf("DESCRIPTION attr retrieved: %s", body.c_str());


	//
	// Now, run checks and convert into Barry object
	//


	Barry::Memo &rec = m_BarryMemo;
	rec.SetIds(Barry::Memo::GetDefaultRecType(), RecordId);

	rec.Title = title;
	rec.Body = body;

	std::ostringstream oss;
	m_BarryMemo.Dump(oss);
	trace.logf("ToBarry, resulting Barry record: %s", oss.str().c_str());
	return m_BarryMemo;
}

// Transfers ownership of m_gMemoData to the caller.
char* vJournal::ExtractVJournal()
{
	char *ret = m_gJournalData;
	m_gJournalData = 0;
	return ret;
}

void vJournal::Clear()
{
	vBase::Clear();
	m_vJournalData.clear();
	m_BarryMemo.Clear();

	if( m_gJournalData ) {
		g_free(m_gJournalData);
		m_gJournalData = 0;
	}
}



//////////////////////////////////////////////////////////////////////////////
//

VJournalConverter::VJournalConverter()
	: m_Data(0)
{
}

VJournalConverter::VJournalConverter(uint32_t newRecordId)
	: m_Data(0),
	m_RecordId(newRecordId)
{
}

VJournalConverter::~VJournalConverter()
{
	if( m_Data )
		g_free(m_Data);
}

// Transfers ownership of m_Data to the caller
char* VJournalConverter::ExtractData()
{
	Trace trace("VMemoConverter::ExtractData");
	char *ret = m_Data;
	m_Data = 0;
	return ret;
}

bool VJournalConverter::ParseData(const char *data)
{
	Trace trace("VJournalConverter::ParseData");

	try {

		vJournal vjournal;
		m_Memo = vjournal.ToBarry(data, m_RecordId);

	}
	catch( vJournal::ConvertError &ce ) {
		trace.logf("ERROR: vJournal::ConvertError exception: %s", ce.what());
		return false;
	}

	return true;
}

// Barry storage operator
void VJournalConverter::operator()(const Barry::Memo &rec)
{
	Trace trace("VJournalConverter::operator()");

	// Delete data if some already exists
	if( m_Data ) {
		g_free(m_Data);
		m_Data = 0;
	}

	try {

		vJournal vjournal;
		vjournal.ToMemo(rec);
		m_Data = vjournal.ExtractVJournal();

	}
	catch( vJournal::ConvertError &ce ) {
		trace.logf("ERROR: vJournal::ConvertError exception: %s", ce.what());
	}
}

// Barry builder operator
bool VJournalConverter::operator()(Barry::Memo &rec, unsigned int dbId)
{
	Trace trace("VMemoConverter::builder operator()");

	rec = m_Memo;
	return true;
}

// Handles calling of the Barry::Controller to fetch a specific
// record, indicated by index (into the RecordStateTable).
// Returns a g_malloc'd string of data containing the vevent20
// data.  It is the responsibility of the caller to free it.
// This is intended to be passed into the GetChanges() function.
char* VJournalConverter::GetRecordData(BarryEnvironment *env, unsigned int dbId,
				Barry::RecordStateTable::IndexType index)
{
	Trace trace("VMemoConverter::GetRecordData()");

	using namespace Barry;

	VJournalConverter memo2journal;
	RecordParser<Memo, VJournalConverter> parser(memo2journal);
	env->m_pDesktop->GetRecord(dbId, index, parser);
	return memo2journal.ExtractData();
}

bool VJournalConverter::CommitRecordData(BarryEnvironment *env, unsigned int dbId,
	Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
	const char *data, bool add, std::string &errmsg)
{
	Trace trace("VJournalConverter::CommitRecordData()");

	uint32_t newRecordId;
	if( add ) {
		// use given id if possible
		if( recordId && !env->m_JournalSync.m_Table.GetIndex(recordId) ) {
			// recordId is unique and non-zero
			newRecordId = recordId;
		}
		else {
			trace.log("Can't use recommended recordId, generating new one.");
			newRecordId = env->m_JournalSync.m_Table.MakeNewRecordId();
		}
	}
	else {
		newRecordId = env->m_JournalSync.m_Table.StateMap[StateIndex].RecordId;
	}
	trace.logf("newRecordId: %lu", newRecordId);

	VJournalConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << "unable to parse change data for new RecordId: "
		    << newRecordId << " data: " << data;
		errmsg = oss.str();
		trace.logf(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Memo, VJournalConverter> builder(convert);

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

