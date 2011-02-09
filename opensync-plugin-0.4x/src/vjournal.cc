//
// \file	vjournal.cc
//		Conversion routines for vjournals (VCALENDAR, etc)
//

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2006-2011, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <sstream>

using namespace Barry::Sync;

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
	catch( Barry::ConvertError &ce ) {
		trace.logf("ERROR: vjournal:Barry::ConvertError exception: %s", ce.what());
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
	catch( Barry::ConvertError &ce ) {
		trace.logf("ERROR: vjournal:Barry::ConvertError exception: %s", ce.what());
	}
}

// Barry builder operator
bool VJournalConverter::operator()(Barry::Memo &rec, Barry::Builder &)
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
	env->GetDesktop()->GetRecord(dbId, index, parser);
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
	trace.logf("newRecordId: %u", newRecordId);

	VJournalConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << "unable to parse change data for new RecordId: "
		    << newRecordId << " data: " << data;
		errmsg = oss.str();
		trace.log(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Memo, VJournalConverter> builder(convert);

	if( add ) {
		trace.log("adding record");
		env->GetDesktop()->AddRecord(dbId, builder);
	}
	else {
		trace.log("setting record");
		env->GetDesktop()->SetRecord(dbId, StateIndex, builder);
		trace.log("clearing dirty flag");
		env->GetDesktop()->ClearDirty(dbId, StateIndex);
	}

	return true;
}

