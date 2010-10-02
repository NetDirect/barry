//
// \file	vevent.cc
//		Conversion routines for vevents (VCALENDAR, etc)
//

/*
    Copyright (C) 2006-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <barry/vevent.h>
#include "vevent.h"
#include "environment.h"
#include "trace.h"
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <sstream>

using namespace Barry::Sync;

//////////////////////////////////////////////////////////////////////////////
//

VEventConverter::VEventConverter()
	: m_Data(0)
{
}

VEventConverter::VEventConverter(uint32_t newRecordId)
	: m_Data(0),
	m_RecordId(newRecordId)
{
}

VEventConverter::~VEventConverter()
{
	if( m_Data )
		g_free(m_Data);
}

// Transfers ownership of m_Data to the caller
char* VEventConverter::ExtractData()
{
	Trace trace("VEventConverter::ExtractData");
	char *ret = m_Data;
	m_Data = 0;
	return ret;
}

bool VEventConverter::ParseData(const char *data)
{
	Trace trace("VEventConverter::ParseData");

	try {

		vTimeConverter vtc;
		vCalendar vcal(vtc);
		m_Cal = vcal.ToBarry(data, m_RecordId);

	}
	catch( Barry::ConvertError &ce ) {
		trace.logf("ERROR: vevent:Barry::ConvertError exception: %s", ce.what());
		return false;
	}

	return true;
}

bool VEventConverter::MergeData(const Barry::Calendar &origin)
{
	// Save CalendarID value
	// CalendarID field is used to link an entry event to an account mail
	if (origin.CalendarID != m_Cal.CalendarID)
		m_Cal.CalendarID = origin.CalendarID;

	return true;
}

// Barry storage operator
void VEventConverter::operator()(const Barry::Calendar &rec)
{
	Trace trace("VEventConverter::operator()");

	// Delete data if some already exists
	if( m_Data ) {
		g_free(m_Data);
		m_Data = 0;
	}

	// Keep a trace of Calendar object (need to merge with the new event)
	m_Cal = rec;

	try {

		vTimeConverter vtc;
		vCalendar vcal(vtc);
		vcal.ToVCal(rec);
		m_Data = vcal.ExtractVCal();

	}
	catch( Barry::ConvertError &ce ) {
		trace.logf("ERROR: vevent:Barry::ConvertError exception: %s", ce.what());
	}
}

// Barry builder operator
bool VEventConverter::operator()(Barry::Calendar &rec, Barry::Builder &)
{
	Trace trace("VEventConverter::builder operator()");

	rec = m_Cal;
	return true;
}

// Handles calling of the Barry::Controller to fetch a specific
// record, indicated by index (into the RecordStateTable).
// Returns a g_malloc'd string of data containing the vevent20
// data.  It is the responsibility of the caller to free it.
// This is intended to be passed into the GetChanges() function.
char* VEventConverter::GetRecordData(BarryEnvironment *env, unsigned int dbId,
				Barry::RecordStateTable::IndexType index)
{
	Trace trace("VEventConverter::GetRecordData()");

	using namespace Barry;

	VEventConverter cal2event;
	RecordParser<Calendar, VEventConverter> parser(cal2event);
	env->m_pDesktop->GetRecord(dbId, index, parser);
	return cal2event.ExtractData();
}

bool VEventConverter::CommitRecordData(BarryEnvironment *env, unsigned int dbId,
	Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
	const char *data, bool add, std::string &errmsg)
{
	Trace trace("VEventConverter::CommitRecordData()");

	uint32_t newRecordId;
	if( add ) {
		// use given id if possible
		if( recordId && !env->m_CalendarSync.m_Table.GetIndex(recordId) ) {
			// recordId is unique and non-zero
			newRecordId = recordId;
		}
		else {
			trace.log("Can't use recommended recordId, generating new one.");
			newRecordId = env->m_CalendarSync.m_Table.MakeNewRecordId();
		}
	}
	else {
		newRecordId = env->m_CalendarSync.m_Table.StateMap[StateIndex].RecordId;
	}
	trace.logf("newRecordId: %lu", newRecordId);

	VEventConverter convert(newRecordId);
	if( !convert.ParseData(data) ) {
		std::ostringstream oss;
		oss << "unable to parse change data for new RecordId: "
		    << newRecordId << " data: " << data;
		errmsg = oss.str();
		trace.log(errmsg.c_str());
		return false;
	}

	// If we modify a data, we read at first its current value
	// then we merge with the parsed value from the other opensync member
	// Merge function is important because, we have to save some BlackBerry fields.
	// Fix an issue with the new OS release who supports several calendar.
	if( !add ) {
		using namespace Barry;

		VEventConverter cal2event;
		RecordParser<Calendar, VEventConverter> parser(cal2event);
		env->m_pDesktop->GetRecord(dbId, StateIndex, parser);
		Calendar cal = cal2event.GetCalendar();

		convert.MergeData(cal);
	}

	Barry::RecordBuilder<Barry::Calendar, VEventConverter> builder(convert);

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

