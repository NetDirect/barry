//
// \file	vjournal.h
//		Conversion routines for vjournals (VCALENDAR, etc)
//

/*
    Copyright (C) 2008-2009, Nicolas VIVIEN
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

#ifndef __BARRY_SYNC_VJOURNAL_H__
#define __BARRY_SYNC_VJOURNAL_H__

#include "dll.h"
#include "vbase.h"
#include "vformat.h"
#include "r_memo.h"
#include <stdint.h>
#include <string>

namespace Barry { namespace Sync {

//
// vJournal
//
/// Class for converting between RFC 2445 iCalendar data format,
/// and the Barry::Memo class.
///
class BXEXPORT vJournal : public vBase
{
	// data to pass to external requests
	char *m_gJournalData;	// dynamic memory returned by vformat()... can
				// be used directly by the plugin, without
				// overmuch allocation and freeing (see Extract())
	std::string m_vJournalData;	// copy of m_gJournalData, for C++ use
	Barry::Memo m_BarryMemo;

protected:
	bool HasMultipleVJournals() const;

public:
	vJournal();
	~vJournal();

	const std::string&	ToMemo(const Barry::Memo &memo);
	const Barry::Memo&	ToBarry(const char *vjournal, uint32_t RecordId);

	char* ExtractVJournal();

	void Clear();
};

}} // namespace Barry::Sync

#endif

