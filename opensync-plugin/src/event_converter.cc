///
/// \file	event_converter.cc
///		Conversion routines for calendar events, to/from
///		OpenSync's XMLFormats
///

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

#include "event_converter.h"
#include "trace.h"
#include <opensync/opensync-time.h>
#include <sstream>


const char *WeekDays[] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };

unsigned short GetWeekDayIndex(const char *dayname)
{
	for( int i = 0; i < 7; i++ ) {
		if( strcasecmp(dayname, WeekDays[i]) == 0 )
			return i;
	}
	return 0;
}

void RecurToBarryCal()
{
	// FIXME - needs to be implemented

	// GetWeekDayIndex()
}


//
// XmlToCalendar
//
/// Converts an OSyncXMLFormat object into a Barry::Calendar object.
/// On error, the auto_ptr<> will contain a null pointer.
///
/*
std::auto_ptr<Barry::Calendar> XmlToCalendar(OSyncXMLFormat *event)
{
	Trace trace("XmlToCalendar");

	using namespace std;

	Trace trace("vCalendar::ToBarry");

	// we only handle vCalendar data with one vevent block
	if( HasMultipleVEvents() )
		throw ConvertError("vCalendar data contains more than one VEVENT block, unsupported");

	// start fresh
	Clear();

	// store the vCalendar raw data
	m_vCalData = vcal;

	// create format parser structures
	m_format = vformat_new_from_string(vcal);
	if( !m_format )
		throw ConvertError("resource error allocating vformat");

	string start = GetAttr("DTSTART");
	trace.logf("DTSTART attr retrieved: %s", start.c_str());
	string end = GetAttr("DTEND");
	trace.logf("DTEND attr retrieved: %s", end.c_str());
	string subject = GetAttr("SUMMARY");
	trace.logf("SUMMARY attr retrieved: %s", subject.c_str());
	if( subject.size() == 0 ) {
		subject = "<blank subject>";
		trace.logf("ERROR: bad data, blank SUMMARY: %s", vcal);
	}

	if( !start.size() || !end.size() ) {
		// FIXME - DTEND is actually optional!  According to the
		// RFC, a DTSTART with no DTEND should be treated
		// like a "special day" like an anniversary, which occupies
		// no time.
		throw ConvertError("Blank DTSTART or DTEND");
	}

	// FIXME - we are assuming that any non-UTC timestamps
	// in the vcalendar record will be in the current timezone...
	// This is wrong!  fix this later.
	//
	// Also, we current ignore any time zone
	// parameters that might be in the vcalendar format... this
	// must be fixed.
	//
	time_t now = time(NULL);
	int zoneoffset = osync_time_timezone_diff(localtime(&now));

	Barry::Calendar &rec = m_BarryCal;
	rec.SetIds(Barry::Calendar::GetDefaultRecType(), RecordId);
	rec.StartTime = osync_time_vtime2unix(start.c_str(), zoneoffset);
	rec.EndTime = osync_time_vtime2unix(end.c_str(), zoneoffset);
	// FIXME - until notification time is supported, we assume 15 min
	// in advance
	rec.NotificationTime = rec.StartTime - 15 * 60;
	rec.Subject = subject;

	return m_BarryCal;
}
*/

/// Exception class for wrapping OSyncError messages.  Takes ownership
/// of the error, and unref's it on destruction.
class osync_error : public std::runtime_error
{
	OSyncError *m_error;

public:
	explicit osync_error(OSyncError *error)
		: std::runtime_error(get_error_msg(error)),
		m_error(error)
	{
	}

	// handle copying correctly
	osync_error(const osync_error &other)
		: std::runtime_error(other.what())
	{
		operator=(other);
	}

	osync_error& operator=(const osync_error &other)
	{
		m_error = other.m_error;
		osync_error_ref(&m_error);
		return *this;
	}

	~osync_error() throw()
	{
		osync_error_unref(&m_error);
	}

	OSyncError* c_obj() { return m_error; }

	static std::string get_error_msg(OSyncError *error)
	{
		return std::string(osync_error_print(&error));
	}
};

class ConvertError : public std::runtime_error
{
public:
	ConvertError(const std::string &msg) : std::runtime_error(msg) {}
};

class XmlField
{
	OSyncXMLField *m_field;

public:
	XmlField(fXMLFormatPtr &format, const char *fieldname)
	{
		OSyncError *error = NULL;
		m_field = osync_xmlfield_new(format.Get(), fieldname, &error);
		if( !m_field )
			throw osync_error(error);
	}

	XmlField(fXMLFormatPtr &format, const char *fieldname, const char *content)
	{
		OSyncError *error = NULL;
		m_field = osync_xmlfield_new(format.Get(), fieldname, &error);
		if( !m_field )
			throw osync_error(error);
		osync_xmlfield_set_key_value(m_field, "Content", content);
	}

	XmlField(fXMLFormatPtr &format, const char *fieldname, const std::string &content)
	{
		OSyncError *error = NULL;
		m_field = osync_xmlfield_new(format.Get(), fieldname, &error);
		if( !m_field )
			throw osync_error(error);
		osync_xmlfield_set_key_value(m_field, "Content", content.c_str());
	}

	void SetKeyValue(const char *key, const char *value)
	{
		osync_xmlfield_set_key_value(m_field, key, value);
	}

	void SetAttr(const char *name, const char *value)
	{
		osync_xmlfield_set_attr(m_field, name, value);
	}
};

void RecurToXml(fXMLFormatPtr &xml, const Barry::Calendar &cal)
{
	using namespace Barry;
	using namespace std;

	if( !cal.Recurring )
		return;

	XmlField recur(xml, "RecurrenceRule");

	switch( cal.RecurringType )
	{
	case Calendar::Day:		// eg. every day
		recur.SetKeyValue("Frequency", "DAILY");
		break;

	case Calendar::MonthByDate:	// eg. every month on the 12th
					// see: DayOfMonth
		recur.SetKeyValue("Frequency", "MONTHLY");
		{
			ostringstream oss;
			oss << cal.DayOfMonth;
			recur.SetKeyValue("ByMonthDay", oss.str().c_str());
		}
		break;

	case Calendar::MonthByDay:	// eg. every month on 3rd Wed
					// see: DayOfWeek and WeekOfMonth
		recur.SetKeyValue("Frequency", "MONTHLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			recur.SetKeyValue("ByDay", oss.str().c_str());
		}
		break;

	case Calendar::YearByDate:	// eg. every year on March 5
					// see: DayOfMonth and MonthOfYear
		recur.SetKeyValue("Frequency", "YEARLY");
		{
			ostringstream oss;
			oss << cal.MonthOfYear;
			recur.SetKeyValue("ByMonth", oss.str().c_str());
		}
		{
			ostringstream oss;
			oss << cal.DayOfMonth;
			recur.SetKeyValue("ByMonthDay", oss.str().c_str());
		}
		break;

	case Calendar::YearByDay:	// eg. every year on 3rd Wed of Jan
					// see: DayOfWeek, WeekOfMonth, and
					//      MonthOfYear
		recur.SetKeyValue("Frequency", "YEARLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			recur.SetKeyValue("ByDay", oss.str().c_str());

			oss.str("");
			oss << cal.MonthOfYear;
			recur.SetKeyValue("ByMonth", oss.str().c_str());
		}
		break;

	case Calendar::Week:		// eg. every week on Mon and Fri
					// see: WeekDays
		recur.SetKeyValue("Frequency", "WEEKLY");
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
			recur.SetKeyValue("WKST", oss.str().c_str());
		}
		break;

	default:
		throw ConvertError("Unknown RecurringType in Barry Calendar object");
	}

	// add some common parameters
	if( cal.Interval > 1 ) {
		ostringstream oss;
		oss << cal.Interval;
		recur.SetKeyValue("Interval", oss.str().c_str());
	}
	if( !cal.Perpetual ) {
		gStringPtr rend(osync_time_unix2vtime(&cal.RecurringEndTime));
		recur.SetKeyValue("Until", rend.Get());
	}

//	bool AllDayEvent;
//
//	///
//	/// Recurring data
//	///
//	/// Note: interval can be used on all of these recurring types to
//	///       make it happen "every other time" or more, etc.
//	///
//
//	bool Recurring;
//	RecurringCodeType RecurringType;
//	unsigned short Interval;	// must be >= 1
//	time_t RecurringEndTime;	// only pertains if Recurring is true
//					// sets the date and time when
//					// recurrence of this appointment
//					// should no longer occur
//					// If a perpetual appointment, this
//					// is 0xFFFFFFFF in the low level data
//					// Instead, set the following flag.
//	bool Perpetual;			// if true, this will always recur
//	unsigned short TimeZoneCode;	// the time zone originally used
//					// for the recurrence data...
//					// seems to have little use, but
//					// set to your current time zone
//					// as a good default
//
//	unsigned short			// recurring details, depending on type
//		DayOfWeek,		// 0-6
//		WeekOfMonth,		// 1-5
//		DayOfMonth,		// 1-31
//		MonthOfYear;		// 1-12
//	unsigned char WeekDays;		// bitmask, bit 0 = sunday
//
//		#define CAL_WD_SUN	0x01
//		#define CAL_WD_MON	0x02
//		#define CAL_WD_TUE	0x04
//		#define CAL_WD_WED	0x08
//		#define CAL_WD_THU	0x10
//		#define CAL_WD_FRI	0x20
//		#define CAL_WD_SAT	0x40
//

}

//
// CalendarToXml
//
/// Converts a Barry::Calendar object into an OSyncXMLFormat object,
/// and returns the resulting, completed, XML object.  On error,
/// returns 0.
///
/// \exception osync_error for OpenSync related errors,
///		and ConvertError for conversion errors
///
fXMLFormatPtr CalendarToXml(const Barry::Calendar &cal)
{
	Trace trace("CalendarToXml");

	OSyncError *error = NULL;

	fXMLFormatPtr xml( osync_xmlformat_new("event", &error) );
	if( !xml.Get() ) {
		trace.errorf("Cannot create new event XML format: %s",
			osync_error_print(&error));
		osync_error_unref(&error);
		return xml;
	}

	// begin building XML data
	XmlField sum(xml, "Summary", cal.Subject);
	XmlField dsc(xml, "Description", cal.Notes);
	XmlField loc(xml, "Location", cal.Location);

	gStringPtr tz_start(osync_time_unix2vtime(&cal.StartTime));
	gStringPtr tz_end(osync_time_unix2vtime(&cal.EndTime));

	XmlField str(xml, "DateStarted", tz_start.Get());
	XmlField end(xml, "DateEnd", tz_end.Get());
	// FIXME - add a truly globally unique "UID" string?

	gStringPtr tz_notify(osync_time_unix2vtime(&cal.NotificationTime));

	XmlField alarm(xml, "Alarm");
	alarm.SetKeyValue("AlarmAction", "AUDIO");
	// notify must be UTC, when specified in DATE-TIME
	alarm.SetKeyValue("AlarmTrigger", tz_notify.Get());
	alarm.SetAttr("Value", "DATE-TIME");


	if( cal.Recurring ) {
		RecurToXml(xml, cal);
	}

	return xml;
}



/*
bool vCalendar::HasMultipleVEvents() const
{
	int count = 0;
	GList *attrs = vformat_get_attributes(m_format);
	for( ; attrs; attrs = attrs->next ) {
		VFormatAttribute *attr = (VFormatAttribute*) attrs->data;
		if( strcasecmp(vformat_attribute_get_name(attr), "BEGIN") == 0 &&
		    strcasecmp(vformat_attribute_get_nth_value(attr, 0), "VEVENT") == 0 )
		{
			count++;
		}
	}
	return count > 1;
}
*/

