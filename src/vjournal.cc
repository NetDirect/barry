//
// \file	vjournal.cc
//		Conversion routines for vjournals (VCALENDAR, etc)
//

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

#include "vjournal.h"
//#include "trace.h"
#include <stdint.h>
#include <glib.h>
#include <strings.h>
#include <sstream>

namespace Barry { namespace Sync {

//////////////////////////////////////////////////////////////////////////////
// vJournal

vJournal::vJournal(vTimeConverter &vtc)
	: m_vtc(vtc)
	, m_gJournalData(0)
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
//	Trace trace("vJournal::ToMemo");
	std::ostringstream oss;
	memo.Dump(oss);
//	trace.logf("ToMemo, initial Barry record: %s", oss.str().c_str());

	// start fresh
	Clear();
	SetFormat( b_vformat_new() );
	if( !Format() )
		throw ConvertError("resource error allocating vformat");

	// store the Barry object we're working with
	m_BarryMemo = memo;

	// RFC section 4.8.7.2 requires DTSTAMP in all VEVENT, VTODO,
	// VJOURNAL, and VFREEBUSY calendar components, and it must be
	// in UTC.  DTSTAMP holds the timestamp of when the iCal object itself
	// was created, not when the object was created in the device or app.
	// So, find out what time it is "now".
	time_t now = time(NULL);

	// begin building vJournal data
	AddAttr(NewAttr("PRODID", "-//OpenSync//NONSGML Barry Memo Record//EN"));
	AddAttr(NewAttr("BEGIN", "VJOURNAL"));
	AddAttr(NewAttr("DTSTAMP", m_vtc.unix2vtime(&now).c_str())); // see note above
	AddAttr(NewAttr("SEQUENCE", "0"));
	AddAttr(NewAttr("SUMMARY", memo.Title.c_str()));
	AddAttr(NewAttr("DESCRIPTION", memo.Body.c_str()));
	AddAttr(NewAttr("CATEGORIES", ToStringList(memo.Categories).c_str()));


	// FIXME - add a truly globally unique "UID" string?

	AddAttr(NewAttr("END", "VJOURNAL"));

	// generate the raw VJOURNAL data
	m_gJournalData = b_vformat_to_string(Format(), VFORMAT_NOTE);
	m_vJournalData = m_gJournalData;

//	trace.logf("ToMemo, resulting vjournal data: %s", m_vJournalData.c_str());
	return m_vJournalData;
}

// Main conversion routine for converting from vJournal data string
// to a Barry::Memo object.
const Barry::Memo& vJournal::ToBarry(const char *vjournal, uint32_t RecordId)
{
	using namespace std;

//	Trace trace("vJournal::ToBarry");
//	trace.logf("ToBarry, working on vmemo data: %s", vjournal);

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
//	trace.logf("SUMMARY attr retrieved: %s", title.c_str());
	if( title.size() == 0 ) {
		title = "<blank subject>";
//		trace.logf("ERROR: bad data, blank SUMMARY: %s", vjournal);
	}

	string body = GetAttr("DESCRIPTION", "/vjournal");
//	trace.logf("DESCRIPTION attr retrieved: %s", body.c_str());


	//
	// Now, run checks and convert into Barry object
	//


	Barry::Memo &rec = m_BarryMemo;
	rec.SetIds(Barry::Memo::GetDefaultRecType(), RecordId);

	rec.Title = title;
	rec.Body = body;
	rec.Categories = GetValueVector("CATEGORIES","/vjournal");

	std::ostringstream oss;
	m_BarryMemo.Dump(oss);
//	trace.logf("ToBarry, resulting Barry record: %s", oss.str().c_str());
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

}} // namespace Barry::Sync

