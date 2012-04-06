///
/// \file	vtodo.cc
///		Conversion routines for vtodos (VCALENDAR, etc)
///

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
    Copyright (C) 2006-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "vtodo.h"
//#include "trace.h"
#include <stdint.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <sstream>

namespace Barry { namespace Sync {

//////////////////////////////////////////////////////////////////////////////
// Utility functions

namespace {
	static void ToLower(std::string &str)
	{
		size_t i = 0;
		while( i < str.size() ) {
			str[i] = tolower(str[i]);
			i++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// vTodo

vTodo::vTodo(vTimeConverter &vtc)
	: m_vtc(vtc)
	, m_gTodoData(0)
{
}

vTodo::~vTodo()
{
	if( m_gTodoData ) {
		g_free(m_gTodoData);
	}
}

bool vTodo::HasMultipleVTodos() const
{
	int count = 0;
	b_VFormat *format = const_cast<b_VFormat*>(Format());
	GList *attrs = format ? b_vformat_get_attributes(format) : 0;
	for( ; attrs; attrs = attrs->next ) {
		b_VFormatAttribute *attr = (b_VFormatAttribute*) attrs->data;
		if( strcasecmp(b_vformat_attribute_get_name(attr), "BEGIN") == 0 &&
		    strcasecmp(b_vformat_attribute_get_nth_value(attr, 0), "VTODO") == 0 )
		{
			count++;
		}
	}
	return count > 1;
}


// Main conversion routine for converting from Barry::Task to
// a vTodo string of data.
const std::string& vTodo::ToTask(const Barry::Task &task)
{
//	Trace trace("vTodo::ToTask");
	std::ostringstream oss;
	task.Dump(oss);
//	trace.logf("ToTask, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryTask = task;

	// RFC section 4.8.7.2 requires DTSTAMP in all VEVENT, VTODO,
	// VJOURNAL, and VFREEBUSY calendar components, and it must be
	// in UTC.  DTSTAMP holds the timestamp of when the iCal object itself
	// was created, not when the object was created in the device or app.
	// So, find out what time it is "now".
	time_t now = time(NULL);

	// begin building vCalendar data
	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Task Record//EN"));
	AddAttr(NewAttr("BEGIN", "VTODO"));
	AddAttr(NewAttr("DTSTAMP", m_vtc.unix2vtime(&now).c_str())); // see note above
	AddAttr(NewAttr("SEQUENCE", "0"));
	AddAttr(NewAttr("SUMMARY", task.Summary.c_str()));
	AddAttr(NewAttr("DESCRIPTION", task.Notes.c_str()));
	AddAttr(NewAttr("CATEGORIES", ToStringList(task.Categories).c_str()));

	// Status
	if (task.StatusFlag == Barry::Task::InProgress)
		AddAttr(NewAttr("STATUS", "IN-PROCESS"));
	else if (task.StatusFlag == Barry::Task::Completed)
		AddAttr(NewAttr("STATUS", "COMPLETED"));
	else if (task.StatusFlag == Barry::Task::Deferred)
		AddAttr(NewAttr("STATUS", "CANCELLED"));
	else if (task.StatusFlag == Barry::Task::Waiting)
		AddAttr(NewAttr("STATUS", "NEEDS-ACTION"));

	// Priority
	if (task.PriorityFlag == Barry::Task::High)
		AddAttr(NewAttr("PRIORITY", "3"));
	else if (task.PriorityFlag == Barry::Task::Normal)
		AddAttr(NewAttr("PRIORITY", "5"));
	else
		AddAttr(NewAttr("PRIORITY", "7"));

	// StartTime
	if( task.StartTime.Time ) {
		AddAttr(NewAttr("DTSTART",
			m_vtc.unix2vtime(&task.StartTime.Time).c_str()));
	}

	// DueTime DueFlag
	if( task.DueTime.IsValid() ) {
		AddAttr(NewAttr("DUE",
			m_vtc.unix2vtime(&task.DueTime.Time).c_str()));
	}

	// FIXME - add a truly globally unique "UID" string?

	AddAttr(NewAttr("END", "VTODO"));

	// generate the raw VTODO data
	m_gTodoData = b_vformat_to_string(Format(), VFORMAT_TODO_20);
	m_vTodoData = m_gTodoData;

//	trace.logf("ToTask, resulting vtodo data: %s", m_vTodoData.c_str());
	return m_vTodoData;
}

// Main conversion routine for converting from vTodo data string
// to a Barry::Task object.
const Barry::Task& vTodo::ToBarry(const char *vtodo, uint32_t RecordId)
{
	using namespace std;

//	Trace trace("vTodo::ToBarry");
//	trace.logf("ToBarry, working on vtodo data: %s", vtodo);

	// we only handle vTodo data with one vtodo block
	if( HasMultipleVTodos() )
		throw ConvertError("vCalendar data contains more than one VTODO block, unsupported");

	// start fresh
	Clear();

	// store the vTodo raw data
	m_vTodoData = vtodo;

	// create format parser structures
	SetFormat( b_vformat_new_from_string(vtodo) );
	if( !Format() )
		throw ConvertError("resource error allocating vtodo");

	string summary = GetAttr("SUMMARY", "/vtodo");
//	trace.logf("SUMMARY attr retrieved: %s", summary.c_str());
	if( summary.size() == 0 ) {
		summary = "<blank subject>";
//		trace.logf("ERROR: bad data, blank SUMMARY: %s", vtodo);
	}

	string notes = GetAttr("DESCRIPTION", "/vtodo");
//	trace.logf("DESCRIPTION attr retrieved: %s", notes.c_str());

	string status = GetAttr("STATUS", "/vtodo");
//	trace.logf("STATUS attr retrieved: %s", status.c_str());

	string priority = GetAttr("PRIORITY", "/vtodo");
//	trace.logf("PRIORITY attr retrieved: %s", priority.c_str());

	string start = GetAttr("DTSTART", "/vtodo");
//	trace.logf("DTSTART attr retrieved: %s", start.c_str());

	string due = GetAttr("DUE", "/vtodo");
//	trace.logf("DUE attr retrieved: %s", due.c_str());


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

	Barry::Task &rec = m_BarryTask;
	rec.SetIds(Barry::Task::GetDefaultRecType(), RecordId);

	// Categories

	rec.Categories = GetValueVector("CATEGORIES","/vtodo");

	// SUMMARY & DESCRIPTION fields
	rec.Summary = summary;
	rec.Notes = notes;

	// STATUS field
	if (status.size()) {
		ToLower(status);

		const char *s = status.c_str();

		if (strstr(s, "in-process"))
			rec.StatusFlag = Barry::Task::InProgress;
		else if (strstr(s, "completed"))
			rec.StatusFlag = Barry::Task::Completed;
		else if (strstr(s, "cancelled"))
			rec.StatusFlag = Barry::Task::Deferred;
		else if (strstr(s, "needs-action"))
			rec.StatusFlag = Barry::Task::Waiting;
		else
			rec.StatusFlag = Barry::Task::NotStarted;
	}

	// PRIORITY field
	if (priority.size()) {
		ToLower(priority);

		const char *s = priority.c_str();

		const int val = atoi(s);

		if (val < 4)
			rec.PriorityFlag = Barry::Task::High;
		else if (val < 7)
			rec.PriorityFlag = Barry::Task::Normal;
		else
			rec.PriorityFlag = Barry::Task::Low;
	}


	// STARTTIME & DUETIME
	if (start.size()) {
		rec.StartTime.Time = m_vtc.vtime2unix(start.c_str());
	}

	if (due.size()) {
		rec.DueTime.Time = m_vtc.vtime2unix(due.c_str());
	}

	std::ostringstream oss;
	m_BarryTask.Dump(oss);
//	trace.logf("ToBarry, resulting Barry record: %s", oss.str().c_str());
	return m_BarryTask;
}

// Transfers ownership of m_gTaskData to the caller.
char* vTodo::ExtractVTodo()
{
	char *ret = m_gTodoData;
	m_gTodoData = 0;
	return ret;
}

void vTodo::Clear()
{
	vBase::Clear();
	m_vTodoData.clear();
	m_BarryTask.Clear();

	if( m_gTodoData ) {
		g_free(m_gTodoData);
		m_gTodoData = 0;
	}
}

}} // namespace Barry::Sync

