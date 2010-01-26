///
/// \file	r_recordstate.cc
///		RecordStateTable database record parser class
///

/*
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "record.h"
#include "record-internal.h"
#include "data.h"
#include <sstream>
#include <iomanip>

using namespace std;
using namespace Barry::Protocol;

namespace Barry {

///////////////////////////////////////////////////////////////////////////////
// RecordStateTable class

RecordStateTable::RecordStateTable()
	: m_LastNewRecordId(1)
{
}

RecordStateTable::~RecordStateTable()
{
}

const unsigned char* RecordStateTable::ParseField(const unsigned char *begin,
						  const unsigned char *end)
{
	const RecordStateTableField *field = (const RecordStateTableField *) begin;

	// advance and check size
	begin += sizeof(RecordStateTableField);
	if( begin > end )		// if begin==end, we are ok
		return begin;

	State state;
	state.Index = btohs(field->index);
	state.RecordId = btohl(field->uniqueId);
	state.Dirty = (field->flags & BARRY_RSTF_DIRTY) != 0;
	state.RecType = field->rectype;
	state.Unknown2.assign((const char*)field->unknown2, sizeof(field->unknown2));
	StateMap[state.Index] = state;

	return begin;
}

void RecordStateTable::Parse(const Data &data)
{
	size_t offset = 12;	// skipping the unknown 2 bytes at start

	if( offset >= data.GetSize() )
		return;

	const unsigned char *begin = data.GetData() + offset;
	const unsigned char *end = data.GetData() + data.GetSize();

	while( begin < end )
		begin = ParseField(begin, end);
}

void RecordStateTable::Clear()
{
	StateMap.clear();
	m_LastNewRecordId = 1;
}

// Searches the StateMap table for RecordId, and returns the "index"
// in the map if found.  Returns true if found, false if not.
// pFoundIndex can be null if only the existence of the index is desired
bool RecordStateTable::GetIndex(uint32_t RecordId, IndexType *pFoundIndex) const
{
	StateMapType::const_iterator i = StateMap.begin();
	for( ; i != StateMap.end(); ++i ) {
		if( i->second.RecordId == RecordId ) {
			if( pFoundIndex )
				*pFoundIndex = i->first;
			return true;
		}
	}
	return false;
}

// Generate a new RecordId that is not in the state table.
// Starts at 1 and keeps incrementing until a free one is found.
uint32_t RecordStateTable::MakeNewRecordId() const
{
	// start with next Id
	m_LastNewRecordId++;

	// make sure it doesn't already exist
	StateMapType::const_iterator i = StateMap.begin();
	while( i != StateMap.end() ) {
		if( m_LastNewRecordId == i->second.RecordId ) {
			m_LastNewRecordId++;		// try again
			i = StateMap.begin();		// start over
		}
		else {
			++i;				// next State
		}
	}
	return m_LastNewRecordId;
}

void RecordStateTable::Dump(std::ostream &os) const
{
	ios::fmtflags oldflags = os.setf(ios::right);
	char fill = os.fill(' ');
	bool bPrintAscii = Data::PrintAscii();
	Data::PrintAscii(false);

	os << "  Index  RecordId    Dirty  RecType" << endl;
	os << "-------  ----------  -----  -------" << endl;

	StateMapType::const_iterator b, e = StateMap.end();
	for( b = StateMap.begin(); b != e ; ++b ) {
		const State &state = b->second;

		os.fill(' ');
		os << setbase(10) << setw(7) << state.Index;
		os << "  0x" << setbase(16) << setfill('0') << setw(8) << state.RecordId;
		os << "  " << setfill(' ') << setw(5) << (state.Dirty ? "yes" : "no");
		os << "     0x" << setbase(16) << setfill('0') << setw(2) << state.RecType;
		os << "   " << Data(state.Unknown2.data(), state.Unknown2.size());
	}

	// cleanup the stream
	os.flags(oldflags);
	os.fill(fill);
	Data::PrintAscii(bPrintAscii);
}


} // namespace Barry

