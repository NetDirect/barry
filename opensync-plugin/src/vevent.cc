//
// \file	vevent.cc
//		Conversion routines for vevents (VCALENDAR, etc)
//

/*
    Copyright (C) 2006-2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "vevent.h"
#include "environment.h"
#include "trace.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include <stdint.h>
#include <glib.h>
#include <sstream>


//////////////////////////////////////////////////////////////////////////////
//

VEventConverter::VEventConverter()
	: m_Data(0)
{
}

VEventConverter::VEventConverter(uint32_t newRecordId)
	: m_RecordId(newRecordId)
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

std::string GetAttr(VFormat *format, const char *attrname)
{
	Trace trace("GetAttr");
	trace.logf("getting attr: %s", attrname);

	std::string ret;

	VFormatAttribute *attr = vformat_find_attribute(format, attrname);
	if( attr ) {
		if( vformat_attribute_is_single_valued(attr) ) {
			ret = vformat_attribute_get_value(attr);
		}
		else {
			// FIXME - does this ever happen?
			ret = vformat_attribute_get_nth_value(attr, 0);
		}
	}

	trace.logf("attr value: %s", ret.c_str());

	return ret;
}

bool VEventConverter::ParseData(const char *data)
{
	Trace trace("VEventConverter::ParseData");

	VFormat *format = vformat_new_from_string(data);
	if( !format ) {
		trace.log("vformat parser unable to initialize");
		return false;
	}

	m_start = GetAttr(format, "DTSTART");
	trace.logf("DTSTART attr retrieved: %s", m_start.c_str());
	m_end = GetAttr(format, "DTEND");
	trace.logf("DTEND attr retrieved: %s", m_end.c_str());
	m_subject = GetAttr(format, "SUMMARY");
	trace.logf("SUMMARY attr retrieved: %s", m_subject.c_str());

	bool success = m_start.size() && m_end.size() && m_subject.size();
	vformat_free(format);
	return success;
}

void AddAttr(Trace &trace, VFormat *format, const char *name, const char *value)
{
	trace.logf("adding attr: %s, %s", name, value);

	VFormatAttribute *attr = vformat_attribute_new(NULL, name);
	if( !attr ) {
		trace.log("resource error allocating vformat attribute");
		return;
	}
	vformat_attribute_add_value(attr, value);
	vformat_add_attribute(format, attr);
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

	VFormat *format = vformat_new();
	if( !format ) {
		trace.log("resource error allocating vformat");
		return;
	}


	char *start = osync_time_unix2vtime(&rec.StartTime);
	char *end = osync_time_unix2vtime(&rec.EndTime);

	AddAttr(trace, format, "BEGIN", "VEVENT");
	AddAttr(trace, format, "PRODID",
		"-//OpenSync//NONSGML Barry Calendar Record//EN");
	AddAttr(trace, format, "DTSTART", start);
	AddAttr(trace, format, "DTEND", end);
	AddAttr(trace, format, "SEQUENCE", "0");
	AddAttr(trace, format, "SUMMARY", rec.Subject.c_str());
	// FIXME - need the notification time too... where does that fit in VCALENDAR?
	AddAttr(trace, format, "END", "VEVENT");

	// generate the raw VCALENDAR data
	m_Data = vformat_to_string(format, VFORMAT_EVENT_20);

	// cleanup
	g_free(start);
	g_free(end);
	vformat_free(format);
}

// Barry builder operator
bool VEventConverter::operator()(Barry::Calendar &rec, unsigned int dbId)
{
	Trace trace("VEventConverter::builder operator()");

	// FIXME - we are assuming that any non-UTC timestamps
	// in the vcalendar record will be in the current timezone...
	// Also, ParseData() currently ignores any time zone
	// parameters that might be in the vcalendar format,
	// so we can't base it on input data.
	time_t now = time(NULL);
	int zoneoffset = osync_time_timezone_diff(localtime(&now));

	rec.SetIds(Barry::Calendar::GetDefaultRecType(), m_RecordId);
	rec.StartTime = osync_time_vtime2unix(m_start.c_str(), zoneoffset);
	rec.EndTime = osync_time_vtime2unix(m_end.c_str(), zoneoffset);
	// FIXME - until notification time is supported, we assume 15 min
	// in advance
	rec.NotificationTime = rec.StartTime - 15 * 60;
	rec.Subject = m_subject;
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
	env->m_pCon->GetRecord(dbId, index, parser);
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
		    << newRecordId;
		errmsg = oss.str();
		trace.logf(errmsg.c_str());
		return false;
	}

	Barry::RecordBuilder<Barry::Calendar, VEventConverter> builder(convert);

	if( add ) {
		trace.log("adding record");
		env->m_pCon->AddRecord(dbId, builder);
	}
	else {
		trace.log("setting record");
		env->m_pCon->SetRecord(dbId, StateIndex, builder);
		trace.log("clearing dirty flag");
		env->m_pCon->ClearDirty(dbId, StateIndex);
	}

	return true;
}

