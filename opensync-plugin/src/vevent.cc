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
// vCalendar

vCalendar::vCalendar()
	: m_gCalData(0),
	m_format(0)
{
}

vCalendar::~vCalendar()
{
	if( m_gCalData ) {
		g_free(m_gCalData);
	}

	if( m_format ) {
		vformat_free(m_format);
	}
}

vAttrPtr vCalendar::NewAttr(const char *name)
{
	Trace trace("vCalendar::NewAttr");

	trace.logf("creating valueless attr: %s", name);

	vAttrPtr attr(vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw ConvertError("resource error allocating vformat attribute");
	return attr;
}

vAttrPtr vCalendar::NewAttr(const char *name, const char *value)
{
	Trace trace("vCalendar::NewAttr");

	if( strlen(value) ) {
		trace.logf("attribute '%s' contains no data, skipping", name);
		return vAttrPtr();
	}

	trace.logf("creating attr: %s, %s", name, value);

	vAttrPtr attr(vformat_attribute_new(NULL, name));
	if( !attr.Get() )
		throw ConvertError("resource error allocating vformat attribute");

	vformat_attribute_add_value(attr.Get(), value);
	return attr;
}

void vCalendar::AddAttr(vAttrPtr attr)
{
	Trace trace("vCalendar::AddAttr");

	if( !attr.Get() ) {
		trace.log("attribute contains no data, skipping");
		return;
	}

	vformat_add_attribute(m_format, attr.Extract());
}

void vCalendar::AddParam(vAttrPtr &attr, const char *name, const char *value)
{
	Trace trace("vCalendar::AddParam");

	if( !attr.Get() ) {
		trace.log("attribute pointer contains no data, skipping");
		return;
	}
	if( strlen(value) ) {
		trace.log("parameter value is empty, skipping");
		return;
	}

	VFormatParam *pParam = vformat_attribute_param_new(name);
	vformat_attribute_param_add_value(pParam, value);
	vformat_attribute_add_param(attr.Get(), pParam);
}

const char *vCalendar::WeekDays[] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };

unsigned short vCalendar::GetWeekDayIndex(const char *dayname)
{
	for( int i = 0; i < 7; i++ ) {
		if( strcasecmp(dayname, WeekDays[i]) == 0 )
			return i;
	}
	return 0;
}

void vCalendar::RecurToVCal()
{
	using namespace Barry;
	using namespace std;
	Barry::Calendar &cal = m_BarryCal;

	if( !cal.Recurring )
		return;

	vAttrPtr attr = NewAttr("RRULE");

	switch( cal.RecurringType )
	{
	case Calendar::Day:		// eg. every day
		AddParam(attr, "FREQ", "DAILY");
		break;

	case Calendar::MonthByDate:	// eg. every month on the 12th
					// see: DayOfMonth
		AddParam(attr, "FREQ", "MONTHLY");
		{
			ostringstream oss;
			oss << cal.DayOfMonth;
			AddParam(attr, "BYMONTHDAY", oss.str().c_str());
		}
		break;

	case Calendar::MonthByDay:	// eg. every month on 3rd Wed
					// see: DayOfWeek and WeekOfMonth
		AddParam(attr, "FREQ", "MONTHLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			AddParam(attr, "BYDAY", oss.str().c_str());
		}
		break;

	case Calendar::YearByDate:	// eg. every year on March 5
					// see: DayOfMonth and MonthOfYear
		AddParam(attr, "FREQ", "YEARLY");
		{
			ostringstream oss;
			oss << cal.MonthOfYear;
			AddParam(attr, "BYMONTH", oss.str().c_str());
		}
		{
			ostringstream oss;
			oss << cal.DayOfMonth;
			AddParam(attr, "BYMONTHDAY", oss.str().c_str());
		}
		break;

	case Calendar::YearByDay:	// eg. every year on 3rd Wed of Jan
					// see: DayOfWeek, WeekOfMonth, and
					//      MonthOfYear
		AddParam(attr, "FREQ", "YEARLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			AddParam(attr, "BYDAY", oss.str().c_str());

			oss.str("");
			oss << cal.MonthOfYear;
			AddParam(attr, "BYMONTH", oss.str().c_str());
		}
		break;

	case Calendar::Week:		// eg. every week on Mon and Fri
					// see: WeekDays
		AddParam(attr, "FREQ", "WEEKLY");
		{
			ostringstream oss;
			for( int i = 0, bm = 1, cnt = 0; i < 7; i++, bm <<= 1 ) {
				if( cal.WeekDays & bm ) {
					if( cnt )
						oss << ",";
					oss << WeekDays[i];
					cnt++;
				}
			}
			AddParam(attr, "BYDAY", oss.str().c_str());
		}
		break;

	default:
		throw ConvertError("Unknown RecurringType in Barry Calendar object");
	}

	// add some common parameters
	if( cal.Interval > 1 ) {
		ostringstream oss;
		oss << cal.Interval;
		AddParam(attr, "INTERVAL", oss.str().c_str());
	}
	if( !cal.Perpetual ) {
		gStringPtr rend(osync_time_unix2vtime(&cal.RecurringEndTime));
		AddParam(attr, "UNTIL", rend.Get());
	}

	AddAttr(attr);

/*
	bool AllDayEvent;

	///
	/// Recurring data
	///
	/// Note: interval can be used on all of these recurring types to
	///       make it happen "every other time" or more, etc.
	///

	bool Recurring;
	RecurringCodeType RecurringType;
	unsigned short Interval;	// must be >= 1
	time_t RecurringEndTime;	// only pertains if Recurring is true
					// sets the date and time when
					// recurrence of this appointment
					// should no longer occur
					// If a perpetual appointment, this
					// is 0xFFFFFFFF in the low level data
					// Instead, set the following flag.
	bool Perpetual;			// if true, this will always recur
	unsigned short TimeZoneCode;	// the time zone originally used
					// for the recurrence data...
					// seems to have little use, but
					// set to your current time zone
					// as a good default

	unsigned short			// recurring details, depending on type
		DayOfWeek,		// 0-6
		WeekOfMonth,		// 1-5
		DayOfMonth,		// 1-31
		MonthOfYear;		// 1-12
	unsigned char WeekDays;		// bitmask, bit 0 = sunday

		#define CAL_WD_SUN	0x01
		#define CAL_WD_MON	0x02
		#define CAL_WD_TUE	0x04
		#define CAL_WD_WED	0x08
		#define CAL_WD_THU	0x10
		#define CAL_WD_FRI	0x20
		#define CAL_WD_SAT	0x40

*/

}

void vCalendar::RecurToBarryCal()
{
	// FIXME - needs to be implemented

	// GetWeekDayIndex()
}

// Main conversion routine for converting from Barry::Calendar to
// a vCalendar string of data.
const std::string& vCalendar::ToVCal(const Barry::Calendar &cal)
{
	// start fresh
	Clear();
	m_format = vformat_new();
	if( !m_format )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryCal = cal;

	// begin building vCalendar data
	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Calendar Record//EN"));
	AddAttr(NewAttr("BEGIN", "VEVENT"));
	AddAttr(NewAttr("SEQUENCE", "0"));
	AddAttr(NewAttr("SUMMARY", cal.Subject.c_str()));
	AddAttr(NewAttr("DESCRIPTION", cal.Notes.c_str()));
	AddAttr(NewAttr("LOCATION", cal.Location.c_str()));

	gStringPtr start(osync_time_unix2vtime(&cal.StartTime));
	gStringPtr end(osync_time_unix2vtime(&cal.EndTime));
	gStringPtr notify(osync_time_unix2vtime(&cal.NotificationTime));

	AddAttr(NewAttr("DTSTART", start.Get()));
	AddAttr(NewAttr("DTEND", end.Get()));
	// FIXME - add a truly globally unique "UID" string?


	AddAttr(NewAttr("BEGIN", "VALARM"));
	AddAttr(NewAttr("ACTION", "AUDIO"));

	// notify must be UTC, when specified in DATE-TIME
	vAttrPtr trigger = NewAttr("TRIGGER", notify.Get());
	AddParam(trigger, "VALUE", "DATE-TIME");
	AddAttr(trigger);

	AddAttr(NewAttr("END", "VALARM"));


	if( cal.Recurring ) {
		RecurToVCal();
	}

	AddAttr(NewAttr("END", "VEVENT"));

	// generate the raw VCALENDAR data
	m_gCalData = vformat_to_string(m_format, VFORMAT_EVENT_20);
	m_vCalData = m_gCalData;

	return m_vCalData;
}

// Main conversion routine for converting from vCalendar data string
// to a Barry::Calendar object.
const Barry::Calendar& vCalendar::ToBarry(const char *vcal)
{
	return m_BarryCal;
}

// Transfers ownership of m_gCalData to the caller.
char* vCalendar::ExtractVCal()
{
	char *ret = m_gCalData;
	m_gCalData = 0;
	return ret;
}

void vCalendar::Clear()
{
	m_vCalData.clear();
	m_BarryCal.Clear();

	if( m_gCalData ) {
		g_free(m_gCalData);
		m_gCalData = 0;
	}

	if( m_format ) {
		vformat_free(m_format);
		m_format = 0;
	}
}



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
	if( m_subject.size() == 0 ) {
		m_subject = "<blank subject>";
		trace.logf("ERROR: bad data, blank SUMMARY: %s", data);
	}

	bool success = m_start.size() && m_end.size();
	vformat_free(format);
	return success;
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

	try {

		vCalendar vcal;
		vcal.ToVCal(rec);
		m_Data = vcal.ExtractVCal();

	}
	catch( vCalendar::ConvertError &ce ) {
		trace.logf("ERROR: vCalendar::ConvertError exception: %s", ce.what());
	}
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
		    << newRecordId << " data: " << data;
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

