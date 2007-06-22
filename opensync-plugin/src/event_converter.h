///
/// \file	event_converter.h
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

#ifndef __BARRY_SYNC_EVENT_CONVERTER_H__
#define __BARRY_SYNC_EVENT_CONVERTER_H__

#include <barry/barry.h>
#include <memory>
#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include "fsmartptr.h"

typedef fSmartPtr<OSyncXMLFormat, OSyncXMLFormat, &osync_xmlformat_unref> fXMLFormatPtr;
typedef fSmartPtr<char, void, &g_free> gStringPtr;

std::auto_ptr<Barry::Calendar> XmlToCalendar(OSyncXMLFormat *event);
fXMLFormatPtr CalendarToXml(const Barry::Calendar &cal);

#endif

