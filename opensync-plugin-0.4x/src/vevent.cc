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

#include <opensync/opensync.h>
#include <opensync/opensync-time.h>

#include "vevent.h"
#include "environment.h"
#include "trace.h"
#include "tosserror.h"
#include "vformat.h"		// comes from opensync, but not a public header yet
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <sstream>
#include <vector>


//////////////////////////////////////////////////////////////////////////////
// vCalendar

vCalendar::vCalendar()
	: m_gCalData(0)
{
}

vCalendar::~vCalendar()
{
	if( m_gCalData ) {
		g_free(m_gCalData);
	}
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

unsigned short vCalendar::GetMonthWeekNumFromBYDAY(const std::string& ByDay)
{
	return atoi(ByDay.substr(0,ByDay.length()-2).c_str());
}

unsigned short vCalendar::GetWeekDayIndexFromBYDAY(const std::string& ByDay)
{
	return GetWeekDayIndex(ByDay.substr(ByDay.length()-2).c_str());
}


bool vCalendar::HasMultipleVEvents() const
{
	int count = 0;
	b_VFormat *format = const_cast<b_VFormat*>(Format());
	GList *attrs = format ? b_vformat_get_attributes(format) : 0;
	for( ; attrs; attrs = attrs->next ) {
		b_VFormatAttribute *attr = (b_VFormatAttribute*) attrs->data;
		if( strcasecmp(b_vformat_attribute_get_name(attr), "BEGIN") == 0 &&
		    strcasecmp(b_vformat_attribute_get_nth_value(attr, 0), "VEVENT") == 0 )
		{
			count++;
		}
	}
	return count > 1;
}

void vCalendar::RecurToVCal()
{
	using namespace Barry;
	using namespace std;
	Barry::Calendar &cal = m_BarryCal;
	Trace trace("vCalendar::RecurToVCal");

	if( !cal.Recurring )
		return;

	vAttrPtr attr = NewAttr("RRULE");

	switch( cal.RecurringType )
	{
	case Calendar::Day:		// eg. every day
		//AddParam(attr, "FREQ", "DAILY");
		AddValue(attr,"FREQ=DAILY");
		break;

	case Calendar::MonthByDate:	// eg. every month on the 12th
					// see: DayOfMonth
		//AddParam(attr, "FREQ", "MONTHLY");
		AddValue(attr,"FREQ=MONTHLY");
		{
			ostringstream oss;
			oss << "BYMONTHDAY=" << cal.DayOfMonth;
			AddValue(attr, oss.str().c_str());
		}
		break;

	case Calendar::MonthByDay:	// eg. every month on 3rd Wed
					// see: DayOfWeek and WeekOfMonth
		AddValue(attr, "FREQ=MONTHLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << "BYDAY=" << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			AddValue(attr, oss.str().c_str());
		}
		break;

	case Calendar::YearByDate:	// eg. every year on March 5
					// see: DayOfMonth and MonthOfYear
		AddValue(attr, "FREQ=YEARLY");
		{
			ostringstream oss;
			oss << "BYMONTH=" << cal.MonthOfYear;
			AddValue(attr, oss.str().c_str());

		}
		{
			ostringstream oss;
			oss << "BYMONTHDAY=" << cal.DayOfMonth;
			AddValue(attr, oss.str().c_str());
		}
		break;

	case Calendar::YearByDay:	// eg. every year on 3rd Wed of Jan
					// see: DayOfWeek, WeekOfMonth, and
					//      MonthOfYear
		AddValue(attr, "FREQ=YEARLY");
		if( cal.DayOfWeek <= 6 ) {	// DayOfWeek is unsigned
			ostringstream oss;
			oss << "BYDAY=" << cal.WeekOfMonth << WeekDays[cal.DayOfWeek];
			AddValue(attr, oss.str().c_str());

			oss.str("");
			oss << "BYMONTH=" << cal.MonthOfYear;
			AddValue(attr, oss.str().c_str());
		}
		break;

	case Calendar::Week:		// eg. every week on Mon and Fri
					// see: WeekDays
		AddValue(attr, "FREQ=WEEKLY");
		{
			ostringstream oss;
			oss << "BYDAY=";
			for( int i = 0, bm = 1, cnt = 0; i < 7; i++, bm <<= 1 ) {
				if( cal.WeekDays & bm ) {
					if( cnt )
						oss << ",";
					oss << WeekDays[i];
					cnt++;
				}
			}
			AddValue(attr, oss.str().c_str());
		}
		break;

	default:
		throw ConvertError("Unknown RecurringType in Barry Calendar object");
	}

	// add some common parameters
	if( cal.Interval > 1 ) {
		ostringstream oss;
		oss << "INTERVAL=" << cal.Interval;
		AddValue(attr, oss.str().c_str());
	}
	if( !cal.Perpetual ) {
		TossError te("RecurToVCal, RecurringEndTime 2 vtime", trace);
		gStringPtr rend(osync_time_unix2vtime(&cal.RecurringEndTime, te));
		if( !te.IsSet() ) {
			ostringstream oss;
			oss << "UNTIL=" << rend.Get();
			AddValue(attr, oss.str().c_str());
		}
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

void vCalendar::RecurToBarryCal(vAttr& rrule, time_t starttime)
{
	using namespace Barry;
	using namespace std;
	Barry::Calendar &cal = m_BarryCal;
	Trace trace("vCalendar::RecurToBarryCal");
	std::map<std::string,unsigned char> pmap;
	pmap["SU"] = CAL_WD_SUN;
	pmap["MO"] = CAL_WD_MON;
	pmap["TU"] = CAL_WD_TUE;
	pmap["WE"] = CAL_WD_WED;
	pmap["TH"] = CAL_WD_THU;
	pmap["FR"] = CAL_WD_FRI;
	pmap["SA"] = CAL_WD_SAT;


	int i=0;
	unsigned int count=0;
	string val;
	std::map<std::string,std::string> args;
	do {
		val=rrule.GetValue(i++);
		if(val.length()==0) {
			break;
		}
		string n=val.substr(0,val.find("="));
		string v=val.substr(val.find("=")+1);
		args[n]=v;
		trace.logf("RecurToBarryCal: |%s|%s|",n.c_str(),v.c_str());
	} while(1);

	// now process the interval.

	time_t now = time(NULL);
	int zoneoffset;
	
	{
		TossError te;
		zoneoffset = osync_time_timezone_diff(localtime(&now), te);
		if( te.IsSet() ) {
			trace.log("RecurToBarryCal: Cannot find local timezone diff... assuming 0");
			zoneoffset = 0;
		}
	}

	cal.Recurring=TRUE;

	if(args.find(string("INTERVAL"))!=args.end()) {
		cal.Interval = atoi(args["INTERVAL"].c_str());
	}
	if(args.find(string("UNTIL"))!=args.end()) {
		TossError te;
		cal.Perpetual = FALSE;
		cal.RecurringEndTime=osync_time_vtime2unix(args["UNTIL"].c_str(), zoneoffset, te);
		if( te.IsSet() ) {
			trace.logf("osync_time_vtime2unix() failed: UNTIL = %s, zoneoffset = %d", args["UNTIL"].c_str(), zoneoffset);
		}
	} else {
		// if we do not also have COUNT, then we must be forerver
		if(args.find(string("COUNT"))==args.end()) {
			cal.Perpetual=TRUE;
		} else {
			// we do have COUNT. This means we won't have UNTIL.
			// So we need to process the RecurringEndTime from
			// the current start date. Set the count level to
			// something other than zero to indicate we need
			// to process it as the exact end date will
			// depend upon the frequency.
			count=atoi(args["COUNT"].c_str());
			if( count == 0 ) {
				throw std::runtime_error("Invalid COUNT in recurring rule: " + args["COUNT"]);
			}
		}
	}

	// we need these if COUNT is true, or if we are a yearly job.

	// TO-DO: we must process COUNT in terms of an end date if we have it.

	// Now deal with the freq

	if(args.find(string("FREQ"))==args.end()) {
		trace.logf("RecurToBarryCal: No frequency specified!");
		return;
	}

	if(args["FREQ"]==string("DAILY")) {
		cal.RecurringType=Calendar::Day;

	} else if(args["FREQ"]==string("WEEKLY")) {
		cal.RecurringType=Calendar::Week;
		// we must have a dayofweek entry
		if(args.find(string("BYDAY"))!=args.end()) {
			std::vector<std::string> v=Tokenize(args["BYDAY"]);
			// iterate along our vector and convert
			for(unsigned int idx=0;idx<v.size();idx++) {
				cal.WeekDays|=pmap[v[idx]];
			}
		} else {
			// handle error here
			trace.logf("RecurToBarryCal: no BYDAY on weekly event");
		}
		if(count) {
			// need to process end date. This is easy
			// for weeks, as a number of weeks can be
			// reduced to seconds simply.
			cal.RecurringEndTime=starttime +((count-1)*60*60*24*7);
		}
	} else if(args["FREQ"]=="MONTHLY") {
		if(args.find(string("BYMONTHDAY"))!=args.end()) {
			cal.RecurringType=Calendar::MonthByDate;
			cal.DayOfMonth=atoi(args["BYMONTHDAY"].c_str());
		} else {
			if(args.find(string("BYDAY"))!=args.end()) {
				cal.RecurringType=Calendar::MonthByDay;
				cal.WeekOfMonth=GetMonthWeekNumFromBYDAY(args["BYDAY"]);
				cal.DayOfWeek=GetWeekDayIndexFromBYDAY(args["BYDAY"]);
			} else {
				trace.logf("RecurToBarryCal: No qualifier on MONTHLY freq");
			}
		}
		if(count) {
			// Nasty. We need to convert to struct tm,
			// do some modulo-12 addition then back
			// to time_t
			struct tm datestruct;
			localtime_r(&starttime,&datestruct);
			// now do some modulo-12 on the month and year
			// We could end up with an illegal date if
			// the day of month is >28 and the resulting
			// month falls on a February. We don't need
			// to worry about day of week as mktime()
			// clobbers it.
			datestruct.tm_year += (datestruct.tm_mon+count)/12;
			datestruct.tm_mon = (datestruct.tm_mon+count)%12;
			if(datestruct.tm_mday>28 && datestruct.tm_mon==1) {
				// force it to 1st Mar
				// TODO Potential bug on leap years
				datestruct.tm_mon=2;
				datestruct.tm_mday=1;
			}
			if(datestruct.tm_mday==31 && (datestruct.tm_mon==8 ||
						      datestruct.tm_mon==3 ||
						      datestruct.tm_mon==5 ||
										  datestruct.tm_mon==10)) {
				datestruct.tm_mon+=1;
				datestruct.tm_mday=1;
			}
			cal.RecurringEndTime=mktime(&datestruct);
		}
	} else if(args["FREQ"]=="YEARLY") {
		if(args.find(string("BYMONTH"))!=args.end()) {
			cal.MonthOfYear=atoi(args["BYMONTH"].c_str());
			if(args.find(string("BYMONTHDAY"))!=args.end()) {
				cal.RecurringType=Calendar::YearByDate;
				cal.DayOfMonth=atoi(args["BYMONTHDAY"].c_str());
			} else {
				if(args.find(string("BYDAY"))!=args.end()) {
					cal.RecurringType=Calendar::YearByDay;
					cal.WeekOfMonth=GetMonthWeekNumFromBYDAY(args["BYDAY"]);
					cal.DayOfWeek=GetWeekDayIndexFromBYDAY(args["BYDAY"]);
				} else {
					trace.logf("RecurToBarryCal: No qualifier on YEARLY freq");
				}
			}
		} else {
			// otherwise use the start date and translate
			// to a BYMONTHDAY.
			// cal.StartTime has already been processed
			// when we get here we need month of year,
			// and day of month.
			struct tm datestruct;
			localtime_r(&starttime,&datestruct);
			cal.RecurringType=Calendar::YearByDate;
			cal.MonthOfYear=datestruct.tm_mon;
			cal.DayOfMonth=datestruct.tm_mday;
		}
		if(count) {
			// convert to struct tm, then simply add to the year.
			struct tm datestruct;
			localtime_r(&starttime,&datestruct);
			datestruct.tm_year += count;
			cal.RecurringEndTime=mktime(&datestruct);
		}
	}

//	unsigned char WeekDays;		// bitmask, bit 0 = sunday
//
//		#define CAL_WD_SUN	0x01
//		#define CAL_WD_MON	0x02
//		#define CAL_WD_TUE	0x04
//		#define CAL_WD_WED	0x08
//		#define CAL_WD_THU	0x10
//		#define CAL_WD_FRI	0x20
//		#define CAL_WD_SAT	0x40
}

// Main conversion routine for converting from Barry::Calendar to
// a vCalendar string of data.
const std::string& vCalendar::ToVCal(const Barry::Calendar &cal)
{
	Trace trace("vCalendar::ToVCal");
	std::ostringstream oss;
	cal.Dump(oss);
	trace.logf("ToVCal, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
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

	// grab Start, End, and Notification time strings (with error checking)
	TossError te;
	gStringPtr start(osync_time_unix2vtime(&cal.StartTime, te));
	if( te.IsSet() )
		trace.logf("osync_time_unix2vtime failed: StartTime = %ld", (long)cal.StartTime);

	te.Clear();
	gStringPtr end(osync_time_unix2vtime(&cal.EndTime, te));
	if( te.IsSet() )
		trace.logf("osync_time_unix2vtime failed: EndTime = %ld", (long)cal.EndTime);

	te.Clear();
	gStringPtr notify(osync_time_unix2vtime(&cal.NotificationTime, te));
	if( te.IsSet() )
		trace.logf("osync_time_unix2vtime failed: NotificationTime = %ld", (long)cal.NotificationTime);

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
	m_gCalData = b_vformat_to_string(Format(), VFORMAT_EVENT_20);
	m_vCalData = m_gCalData;

	trace.logf("ToVCal, resulting vcal data: %s", m_vCalData.c_str());
	return m_vCalData;
}

// Main conversion routine for converting from vCalendar data string
// to a Barry::Calendar object.
const Barry::Calendar& vCalendar::ToBarry(const char *vcal, uint32_t RecordId)
{
	using namespace std;

	Trace trace("vCalendar::ToBarry");
	trace.logf("ToBarry, working on vcal data: %s", vcal);

	// we only handle vCalendar data with one vevent block
	if( HasMultipleVEvents() )
		throw ConvertError("vCalendar data contains more than one VEVENT block, unsupported");

	// start fresh
	Clear();

	// store the vCalendar raw data
	m_vCalData = vcal;

	// create format parser structures
	SetFormat( b_vformat_new_from_string(vcal) );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	string start = GetAttr("DTSTART", "/vevent");
	trace.logf("DTSTART attr retrieved: %s", start.c_str());
	string end = GetAttr("DTEND", "/vevent");
	trace.logf("DTEND attr retrieved: %s", end.c_str());
	string subject = GetAttr("SUMMARY", "/vevent");
	trace.logf("SUMMARY attr retrieved: %s", subject.c_str());
	if( subject.size() == 0 ) {
		subject = "<blank subject>";
		trace.logf("ERROR: bad data, blank SUMMARY: %s", vcal);
	}
	vAttr trigger_obj = GetAttrObj("TRIGGER", 0, "/valarm");

	string location = GetAttr("LOCATION", "/vevent");
	trace.logf("LOCATION attr retrieved: %s", location.c_str());

	string notes = GetAttr("DESCRIPTION", "/vevent");
	trace.logf("DESCRIPTION attr retrieved: %s", notes.c_str());

	vAttr rrule = GetAttrObj("RRULE",0,"/vevent");


	//
	// Now, run checks and convert into Barry object
	//


	// FIXME - we are assuming that any non-UTC timestamps
	// in the vcalendar record will be in the current timezone...
	// This is wrong!  fix this later.
	//
	// Also, we current ignore any time zone
	// parameters that might be in the vcalendar format... this
	// must be fixed.
	//
	time_t now = time(NULL);
	int zoneoffset;
	{
		TossError te;
		zoneoffset = osync_time_timezone_diff(localtime(&now), te);
		if( te.IsSet() ) {
			trace.log("ToBarry: Cannot find local timezone diff... assuming 0");
			zoneoffset = 0;
		}
	}

	Barry::Calendar &rec = m_BarryCal;
	rec.SetIds(Barry::Calendar::GetDefaultRecType(), RecordId);

	if( !start.size() )
		throw ConvertError("Blank DTSTART");
	TossError te("ToBarry, StartTime 2 unix", trace);
	rec.StartTime = osync_time_vtime2unix(start.c_str(), zoneoffset, te);

	if( !end.size() ) {
		// DTEND is actually optional!  According to the
		// RFC, a DTSTART with no DTEND should be treated
		// like a "special day" like an anniversary, which occupies
		// no time.
		//
		// Since the Blackberry doesn't really map well to this
		// case, we'll set the end time to 1 day past start.
		//
		rec.EndTime = rec.StartTime + 24 * 60 * 60;
	}
	else {
		TossError te("ToBarry, EndTime 2 unix", trace);
		rec.EndTime = osync_time_vtime2unix(end.c_str(), zoneoffset, te);
	}

	rec.Subject = subject;
	rec.Location = location;
	rec.Notes = notes;

	if(rrule.Get()) {
		RecurToBarryCal(rrule, rec.StartTime);
	}

	// convert trigger time into notification time
	// assume no notification, by default
	rec.NotificationTime = 0;
	if( trigger_obj.Get() ) {
		string trigger_type = trigger_obj.GetParam("VALUE");
		string trigger = trigger_obj.GetValue();

		if( trigger.size() == 0 ) {
			trace.logf("ERROR: no TRIGGER found in calendar entry, assuming notification time as 15 minutes before start.");
		}
		else if( trigger_type == "DATE-TIME" ) {
			TossError te("ToBarry, NotificationTime 2 unix", trace);
			rec.NotificationTime = osync_time_vtime2unix(trigger.c_str(), zoneoffset, te);
		}
		else if( trigger_type == "DURATION" || trigger_type.size() == 0 ) {
			// default is DURATION (RFC 4.8.6.3)
			string related = trigger_obj.GetParam("RELATED");

			// default to relative to start time
			time_t *relative = &rec.StartTime;
			if( related == "END" )
				relative = &rec.EndTime;

			rec.NotificationTime = *relative + osync_time_alarmdu2sec(trigger.c_str());
		}
		else {
			throw ConvertError("Unknown TRIGGER VALUE");
		}
	}
	else {
		trace.logf("ERROR: no TRIGGER found in calendar entry, assuming notification time as 15 minutes before start.");
	}

	std::ostringstream oss;
	m_BarryCal.Dump(oss);
	trace.logf("ToBarry, resulting Barry record: %s", oss.str().c_str());
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
	vBase::Clear();
	m_vCalData.clear();
	m_BarryCal.Clear();

	if( m_gCalData ) {
		g_free(m_gCalData);
		m_gCalData = 0;
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

bool VEventConverter::ParseData(const char *data)
{
	Trace trace("VEventConverter::ParseData");

	try {

		vCalendar vcal;
		m_Cal = vcal.ToBarry(data, m_RecordId);

	}
	catch( vCalendar::ConvertError &ce ) {
		trace.logf("ERROR: vCalendar::ConvertError exception: %s", ce.what());
		return false;
	}

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
	trace.logf("newRecordId: %u", newRecordId);

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

