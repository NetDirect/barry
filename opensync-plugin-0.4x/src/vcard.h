///
/// \file	vcard.h
///		Conversion routines for vcards
///

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

#ifndef __BARRYSYNC_VCARD_H__
#define __BARRYSYNC_VCARD_H__

#include <barry/barry.h>
#include <barry/barrysync.h>
#include <stdint.h>
#include <string>

// forward declarations
class BarryEnvironment;

class VCardConverter
{
	char *m_Data;
	Barry::Contact m_Contact;
	uint32_t m_RecordId;

public:
	VCardConverter();
	explicit VCardConverter(uint32_t newRecordId);
	~VCardConverter();

	// Transfers ownership of m_Data to the caller
	char* ExtractData();

	// Parses vevent data
	bool ParseData(const char *data);

	// Barry storage operator
	void operator()(const Barry::Contact &rec);

	// Barry builder operator
	bool operator()(Barry::Contact &rec, Barry::Builder &);

	// Handles calling of the Barry::Controller to fetch a specific
	// record, indicated by index (into the RecordStateTable).
	// Returns a g_malloc'd string of data containing the vevent20
	// data.  It is the responsibility of the caller to free it.
	// This is intended to be passed into the GetChanges() function.
	static char* GetRecordData(BarryEnvironment *env, unsigned int dbId,
		Barry::RecordStateTable::IndexType index);

	// Handles either adding or overwriting a calendar record,
	// given vevent20 data in data, and the proper environmebnt,
	// dbId, StateIndex.  Set add to true if adding.
	static bool CommitRecordData(BarryEnvironment *env, unsigned int dbId,
		Barry::RecordStateTable::IndexType StateIndex, uint32_t recordId,
		const char *data, bool add, std::string &errmsg);
};

#endif

