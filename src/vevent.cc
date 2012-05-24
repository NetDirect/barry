///
/// \file	vevent.cc
///		Conversion routines for vevents (VCALENDAR, etc)
///

/*
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2009, Dr J A Gow <J.A.Gow@wellfrazzled.com>

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
//#include "trace.h"
#include "log.h"
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <stdlib.h>
#include <sstream>
#include <string>

using namespace std;

namespace Barry { namespace Sync {

//////////////////////////////////////////////////////////////////////////////
// vCalendar

vCalendar::vCalendar(vTimeConverter &vtc)
	: m_vtc(vtc)
	, m_gCalData(0)
{
}

vCalendar::~vCalendar()
{
	if( m_gCalData ) {
		g_free(m_gCalData);
	}
}

const char *vCalendar::WeekDays[] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };

uint16_t vCalendar::GetWeekDayIndex(const char *dayname)
{
	for( int i = 0; i < 7; i++ ) {
		if( strcasecmp(dayname, WeekDays[i]) == 0 )
			return i;
	}
	return 0;
}

void vCalendar::CheckUnsupportedArg(const ArgMapType &args,
					const std::string &name)
{
	if( args.find(name) != args.end() ) {
		barrylog("ERROR: recurrence rule contains " << name << ", unsupported by Barry. MIME conversion will be incorrect.");
		barryverbose("Record data so far:\n" << m_BarryCal);
	}
}

uint16_t vCalendar::GetMonthWeekNumFromBYDAY(const std::string& ByDay)
{
	return atoi(ByDay.substr(0,ByDay.length()-2).c_str());
}

uint16_t vCalendar::GetWeekDayIndexFromBYDAY(const std::string& ByDay)
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

	if( !cal.Recurring )
		return;

	vAttrPtr attr = NewAttr("RRULE");

	switch( cal.RecurringType )
	{
	case Calendar::Day:		// eg. every day
		AddValue(attr,"FREQ=DAILY");
		break;

	case Calendar::MonthByDate:	// eg. every month on the 12th
					// see: DayOfMonth
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
		ostringstream oss;
		oss << "UNTIL=" << m_vtc.unix2vtime(&cal.RecurringEndTime.Time);
		AddValue(attr, oss.str().c_str());
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
	uint16_t Interval;		// must be >= 1
	time_t RecurringEndTime;	// only pertains if Recurring is true
					// sets the date and time when
					// recurrence of this appointment
					// should no longer occur
					// If a perpetual appointment, this
					// is 0xFFFFFFFF in the low level data
					// Instead, set the following flag.
	bool Perpetual;			// if true, this will always recur
	uint16_t TimeZoneCode;		// the time zone originally used
					// for the recurrence data...
					// seems to have little use, but
					// set to your current time zone
					// as a good default

	uint16_t			// recurring details, depending on type
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

namespace {
	struct tm GetConstLocalTime(const time_t &t)
	{
		struct tm datestruct;
		localtime_r(&t, &datestruct);
		return datestruct;
	}
}

void vCalendar::RecurToBarryCal(vAttr& rrule, time_t starttime)
{
	using namespace Barry;
	using namespace std;
	Barry::Calendar &cal = m_BarryCal;
//	Trace trace("vCalendar::RecurToBarryCal");
	std::map<std::string,unsigned char> pmap;
	pmap["SU"] = CAL_WD_SUN;
	pmap["MO"] = CAL_WD_MON;
	pmap["TU"] = CAL_WD_TUE;
	pmap["WE"] = CAL_WD_WED;
	pmap["TH"] = CAL_WD_THU;
	pmap["FR"] = CAL_WD_FRI;
	pmap["SA"] = CAL_WD_SAT;

	const struct tm datestruct = GetConstLocalTime(starttime);

	int i=0;
	unsigned int count=0;
	string val;
	ArgMapType args;
	do {
		val=rrule.GetValue(i++);
		if(val.length()==0) {
			break;
		}
		string n=val.substr(0,val.find("="));
		string v=val.substr(val.find("=")+1);
		args[n]=v;
//		trace.logf("RecurToBarryCal: |%s|%s|",n.c_str(),v.c_str());
	} while(1);

	// now process the interval.
	cal.Recurring=TRUE;

	if(args.find(string("INTERVAL"))!=args.end()) {
		int interval = atoi(args["INTERVAL"].c_str());
		if( interval < 1 ) {
			// force to at least 1, for math below
			interval = 1;
		}
		cal.Interval = interval;
	}
	else {
		// default to 1, for the math below.
		// RecurBase::Clear() does this for us as well, but
		// best to be safe
		cal.Interval = 1;
	}
	if(args.find(string("UNTIL"))!=args.end()) {
		cal.Perpetual = FALSE;
		cal.RecurringEndTime.Time = m_vtc.vtime2unix(args["UNTIL"].c_str());
		if( cal.RecurringEndTime.Time == (time_t)-1 ) {
//			trace.logf("osync_time_vtime2unix() failed: UNTIL = %s, zoneoffset = %d", args["UNTIL"].c_str(), zoneoffset);
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

	// warn the user about unsupported arguments
	CheckUnsupportedArg(args, "BYSETPOS");// FIXME - theorectically supportable
	CheckUnsupportedArg(args, "BYYEARDAY");
	CheckUnsupportedArg(args, "BYWEEKNO");
	CheckUnsupportedArg(args, "WKST");
	CheckUnsupportedArg(args, "BYSECOND");
	CheckUnsupportedArg(args, "BYMINUTE");
	CheckUnsupportedArg(args, "BYHOUR");

	// Now deal with the freq

	if(args.find(string("FREQ"))==args.end()) {
//		trace.logf("RecurToBarryCal: No frequency specified!");
		return;
	}

	if(args["FREQ"]==string("DAILY")) {
		cal.RecurringType=Calendar::Day;

		if(count) {
			// add count-1*interval days to find the end time:
			// i.e. if starting on 2012/01/01 and going
			// for 3 days, then the last day will be
			// 2012/01/03.
			//
			// For intervals, the count is every interval days,
			// so interval of 2 means 2012/01/01, 2012/01/03, etc.
			// and the calculation still works.
			cal.RecurringEndTime.Time =
				starttime + (count-1) * cal.Interval * 24*60*60;
		}
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
			// we must have at least one day selected, and if no
			// BYDAY is selected, use the start time's day
			cal.WeekDays = pmap[WeekDays[datestruct.tm_wday]];

			barrylog("Warning: WEEKLY VEVENT without a day selected. Assuming day of start time.");
			barryverbose("Record data so far:\n" << cal);
		}

		if(count) {
			// need to process end date. This is easy
			// for weeks, as a number of weeks can be
			// reduced to seconds simply.
			cal.RecurringEndTime.Time =
				starttime + (count-1) * cal.Interval * 60*60*24*7;
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
				// must have a recurring type, so assume
				// that monthly means every day on the day
				// of month specified by starttime
				cal.RecurringType = Calendar::MonthByDate;
				cal.DayOfMonth = datestruct.tm_mday;
				barrylog("Warning: MONTHLY VEVENT without a day type specified (no BYMONTHDAY nor BYDAY). Assuming BYMONTHDAY, using day of start time.");
				barryverbose("Record data so far:\n" << cal);
			}
		}
		if(count) {
			// Nasty. We need to convert to struct tm,
			// do some modulo-12 addition then back
			// to time_t
			struct tm tempdate = datestruct;

			// now do some modulo-12 on the month and year
			// We could end up with an illegal date if
			// the day of month is >28 and the resulting
			// month falls on a February. We don't need
			// to worry about day of week as mktime()
			// clobbers it.
			int add = (count-1) * cal.Interval;
			tempdate.tm_year += (tempdate.tm_mon+add)/12;
			tempdate.tm_mon = (tempdate.tm_mon+add)%12;
			if(tempdate.tm_mday>28 && tempdate.tm_mon==1) {
				// force it to 1st Mar
				// TODO Potential bug on leap years
				tempdate.tm_mon=2;
				tempdate.tm_mday=1;
			}
			if(tempdate.tm_mday==31 && (tempdate.tm_mon==8 ||
						      tempdate.tm_mon==3 ||
						      tempdate.tm_mon==5 ||
										  tempdate.tm_mon==10)) {
				tempdate.tm_mon+=1;
				tempdate.tm_mday=1;
			}
			// Just in case we're crossing DST boundaries,
			// add an hour, to make sure we reach the ending
			// month, in the case of intervals
			tempdate.tm_hour++;
			cal.RecurringEndTime.Time = mktime(&tempdate);
		}
	} else if(args["FREQ"]=="YEARLY") {
		bool need_assumption = true;
		if(args.find(string("BYMONTH"))!=args.end()) {
			cal.MonthOfYear=atoi(args["BYMONTH"].c_str());
			if(args.find(string("BYMONTHDAY"))!=args.end()) {
				cal.RecurringType=Calendar::YearByDate;
				cal.DayOfMonth=atoi(args["BYMONTHDAY"].c_str());
				need_assumption = false;
			} else {
				if(args.find(string("BYDAY"))!=args.end()) {
					cal.RecurringType=Calendar::YearByDay;
					cal.WeekOfMonth=GetMonthWeekNumFromBYDAY(args["BYDAY"]);
					cal.DayOfWeek=GetWeekDayIndexFromBYDAY(args["BYDAY"]);
					need_assumption = false;
				} else {
					// fall through to assumption below...
				}
			}
		}

		if( need_assumption ) {
			// otherwise use the start date and translate
			// to a BYMONTHDAY.
			// cal.StartTime has already been processed
			// when we get here we need month of year,
			// and day of month.
			cal.RecurringType=Calendar::YearByDate;
			cal.MonthOfYear=datestruct.tm_mon;
			cal.DayOfMonth=datestruct.tm_mday;
			barrylog("Warning: YEARLY VEVENT without a day type specified (no BYMONTHDAY nor BYDAY). Assuming BYMONTHDAY, using day and month of start time.");
			barryverbose("Record data so far:\n" << cal);
		}
		if(count) {
			// convert to struct tm, then simply add to the year.
			//
			// Note: intervals do work in the device firmware,
			// but not all of the devices allow you to edit it
			// with their GUI... hmmm... oh well, allow it
			// anyway, and do the multiplication below.
			struct tm tempdate = datestruct;
			tempdate.tm_year += (count-1) * cal.Interval;
			cal.RecurringEndTime.Time = mktime(&tempdate);
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
//	Trace trace("vCalendar::ToVCal");
	std::ostringstream oss;
	cal.Dump(oss);
//	trace.logf("ToVCal, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryCal = cal;

	// RFC section 4.8.7.2 requires DTSTAMP in all VEVENT, VTODO,
	// VJOURNAL, and VFREEBUSY calendar components, and it must be
	// in UTC.  DTSTAMP holds the timestamp of when the iCal object itself
	// was created, not when the object was created in the device or app.
	// So, find out what time it is "now".
	time_t now = time(NULL);

	// begin building vCalendar data
	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Calendar Record//EN"));
	AddAttr(NewAttr("BEGIN", "VEVENT"));
	AddAttr(NewAttr("DTSTAMP", m_vtc.unix2vtime(&now).c_str())); // see note above
	AddAttr(NewAttr("SEQUENCE", "0"));
	AddAttr(NewAttr("SUMMARY", cal.Subject.c_str()));
	AddAttr(NewAttr("DESCRIPTION", cal.Notes.c_str()));
	AddAttr(NewAttr("LOCATION", cal.Location.c_str()));

	string start(m_vtc.unix2vtime(&cal.StartTime.Time));
	string end(m_vtc.unix2vtime(&cal.EndTime.Time));
	string notify(m_vtc.unix2vtime(&cal.NotificationTime.Time));

	// if an all day event, only print the date parts of the string
	if( cal.AllDayEvent && start.find('T') != string::npos ) {
		// truncate start date
		start = start.substr(0, start.find('T'));

		// create end date 1 day in future
		time_t end_t = cal.StartTime.Time + 24 * 60 * 60;
		end = m_vtc.unix2vtime(&end_t);
		end = end.substr(0, end.find('T'));
	}

	AddAttr(NewAttr("DTSTART", start.c_str()));
	AddAttr(NewAttr("DTEND", end.c_str()));
	// FIXME - add a truly globally unique "UID" string?


	AddAttr(NewAttr("BEGIN", "VALARM"));
	AddAttr(NewAttr("ACTION", "AUDIO"));

	// notify must be UTC, when specified in DATE-TIME
	vAttrPtr trigger = NewAttr("TRIGGER", notify.c_str());
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

//	trace.logf("ToVCal, resulting vcal data: %s", m_vCalData.c_str());
	return m_vCalData;
}

// Main conversion routine for converting from vCalendar data string
// to a Barry::Calendar object.
const Barry::Calendar& vCalendar::ToBarry(const char *vcal, uint32_t RecordId)
{
	using namespace std;

//	Trace trace("vCalendar::ToBarry");
//	trace.logf("ToBarry, working on vcal data: %s", vcal);

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
//	trace.logf("DTSTART attr retrieved: %s", start.c_str());
	string end = GetAttr("DTEND", "/vevent");
//	trace.logf("DTEND attr retrieved: %s", end.c_str());
	string subject = GetAttr("SUMMARY", "/vevent");
//	trace.logf("SUMMARY attr retrieved: %s", subject.c_str());
	if( subject.size() == 0 ) {
		subject = "<blank subject>";
//		trace.logf("ERROR: bad data, blank SUMMARY: %s", vcal);
	}
	vAttr trigger_obj = GetAttrObj("TRIGGER", 0, "/valarm");

	string location = GetAttr("LOCATION", "/vevent");
//	trace.logf("LOCATION attr retrieved: %s", location.c_str());

	string notes = GetAttr("DESCRIPTION", "/vevent");
//	trace.logf("DESCRIPTION attr retrieved: %s", notes.c_str());

	vAttr rrule = GetAttrObj("RRULE",0,"/vevent");


	//
	// Now, run checks and convert into Barry object
	//


	// FIXME - we are assuming that any non-UTC timestamps
	// in the vcalendar record will be in the current timezone...
	// This is wrong!  fix this later.
	//
	// Also, we currently ignore any time zone
	// parameters that might be in the vcalendar format... this
	// must be fixed.
	//
	Barry::Calendar &rec = m_BarryCal;
	rec.SetIds(Barry::Calendar::GetDefaultRecType(), RecordId);

	if( !start.size() )
		throw ConvertError("Blank DTSTART");
	rec.StartTime.Time = m_vtc.vtime2unix(start.c_str());

	if( !end.size() ) {
		// DTEND is actually optional!  According to the
		// RFC, a DTSTART with no DTEND should be treated
		// like a "special day" like an anniversary, which occupies
		// no time.
		//
		// Since the Blackberry doesn't really map well to this
		// case, we'll set the end time to 1 day past start.
		//
		rec.EndTime.Time = rec.StartTime.Time + 24 * 60 * 60;
	}
	else {
		rec.EndTime.Time = m_vtc.vtime2unix(end.c_str());
	}

	// check for "all day event" which is specified by a DTSTART
	// and a DTEND with no times, and one day apart
	if( start.find('T') == string::npos && end.size() &&
	    end.find('T') == string::npos &&
	    (rec.EndTime.Time - rec.StartTime.Time) == 24 * 60 * 60 )
	{
		rec.AllDayEvent = true;
	}

	rec.Subject = subject;
	rec.Location = location;
	rec.Notes = notes;

	if(rrule.Get()) {
		RecurToBarryCal(rrule, rec.StartTime.Time);
	}

	// convert trigger time into notification time
	// assume no notification, by default
	rec.NotificationTime.Time = 0;
	if( trigger_obj.Get() ) {
		string trigger_type = trigger_obj.GetParam("VALUE");
		string trigger = trigger_obj.GetValue();

		if( trigger.size() == 0 ) {
//			trace.logf("ERROR: no TRIGGER found in calendar entry, assuming notification time as 15 minutes before start.");
		}
		else if( trigger_type == "DATE-TIME" ) {
			rec.NotificationTime.Time = m_vtc.vtime2unix(trigger.c_str());
		}
		else if( trigger_type == "DURATION" || trigger_type.size() == 0 ) {
			// default is DURATION (RFC 4.8.6.3)
			string related = trigger_obj.GetParam("RELATED");

			// default to relative to start time
			time_t *relative = &rec.StartTime.Time;
			if( related == "END" )
				relative = &rec.EndTime.Time;

			rec.NotificationTime.Time = *relative + m_vtc.alarmduration2sec(trigger.c_str());
		}
		else {
			throw ConvertError("Unknown TRIGGER VALUE");
		}
	}
	else {
//		trace.logf("ERROR: no TRIGGER found in calendar entry, assuming notification time as 15 minutes before start.");
	}

	std::ostringstream oss;
	m_BarryCal.Dump(oss);
//	trace.logf("ToBarry, resulting Barry record: %s", oss.str().c_str());
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

}} // namespace Barry::Sync

