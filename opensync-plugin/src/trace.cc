//
// \file	trace.cc
//		RAII class for trace logging.
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

#include "trace.h"
#include <opensync/opensync.h>
#include <stdarg.h>

//////////////////////////////////////////////////////////////////////////////
// Trace class, private API

void Trace::do_logf(OSyncTraceType type, const char *msg, va_list vl)
{
	char buffer[2048];
	int n = vsnprintf(buffer, sizeof(buffer), t, vl);
	if( n > -1 && n < (int)sizeof(buffer) )
		osync_trace(type, "barry_sync: %s", buffer);
	else
		osync_trace(type, "barry_sync: (trace error, output too long for buffer: %s)", t);
	strcpy(m_buffer, buffer);
}


//////////////////////////////////////////////////////////////////////////////
// Trace class, public API

Trace::Trace(const char *t)
	: text(t),
	tag(0),
	m_error(false)
{
	osync_trace(TRACE_ENTRY, "barry_sync: %s", text);
}

Trace::Trace(const char *t, const char *tag)
	: text(t),
	tag(tag),
	m_error(false)
{
	osync_trace(TRACE_ENTRY, "barry_sync (%s): %s", tag, text);
}

Trace::~Trace()
{
	if( tag )
		osync_trace(m_error ? TRACE_EXIT_ERROR : TRACE_EXIT,
			"barry_sync (%s): %s", tag, text);
	else
		osync_trace(m_error ? TRACE_EXIT_ERROR : TRACE_EXIT,
			"barry_sync: %s", text);
}

void Trace::log(const char *t)
{
	osync_trace(TRACE_INTERNAL, "barry_sync: %s", t);
}

void Trace::logf(const char *t, ...)
{
	va_list vl;
	va_start(vl, t);
	do_logf(TRACE_INTERNAL, t, vl);
	va_end(vl);
}

//
// log(OSyncXMLField *field)
//
/// Log entire contents of field.
///
void Trace::log(OSyncXMLField *field)
{
	int attrcount = osync_xmlfield_get_attr_count(field);
	int keycount = osync_xmlfield_get_key_count(field);

	osync_trace(TRACE_INTERNAL,
		"XMLField: name = '%s', AttrCount = %d, KeyCount = %d",
		osync_xmlfield_get_name(field),
		attrcount,
		keycount);

	for( int i = 0; i < count; i++ ) {
		osync_trace(TRACE_INTERNAL, "   Attr[%d]: '%s' = '%s'",
			i,
			osync_xmlfield_get_nth_attr_name(field, i),
			osync_xmlfield_get_nth_attr_value(field, i));
	}

	for( int i = 0; i < keycount; i++ ) {
		osync_trace(TRACE_INTERNAL, "   Key[%d]: '%s' = '%s'",
			i,
			osync_xmlfield_get_nth_key_name(field, i),
			osync_xmlfield_get_nth_key_value(field, i));
	}
}

void Trace::error(const char *t)
{
	m_error = true;
	osync_trace(TRACE_ERROR, "barry_sync: %s", t);
}

void Trace::errorf(const char *t, ...)
{
	m_error = true;

	va_list vl;
	va_start(vl, t);
	do_logf(TRACE_ERROR, t, vl);
	va_end(vl);
}

