//
// \file	idmap.cc
//		Class that maps opensync UID strings to Blackberry
//		Record ID's and back.
//

/*
    Copyright (C) 2007-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "idmap.h"
#include <fstream>
#include "trace.h"

idmap::idmap()
	: m_blank_uid(""),
	m_blank_rid(0)
{
}

idmap::~idmap()
{
}

bool idmap::Load(const char *filename)
{
	// start fresh
	m_map.clear();

	std::ifstream ifs(filename);
	if( !ifs )
		return false;

	std::string line;
	uint32_t recordId;
	while( ifs ) {
		recordId = 0;
		ifs >> recordId >> std::ws;
		std::getline(ifs, line);
		if( ifs && recordId && line.size() ) {
			Map(line, recordId);
		}
	}
	return ifs.eof();
}

bool idmap::Save(const char *filename) const
{
	std::ofstream ofs(filename);
	if( !ofs )
		return false;

	const_iterator i = m_map.begin();
	for( ; i != m_map.end(); ++i ) {
		ofs << i->second << " " << i->first << std::endl;
	}
	return !ofs.bad() && !ofs.fail();
}

bool idmap::UidExists(const uid_type &uid, const_iterator *it) const
{
	const_iterator i = m_map.find(uid);
	if( it )
		*it = i;
	return i != m_map.end();
}

bool idmap::RidExists(const rid_type &rid, const_iterator *it) const
{
	const_iterator i = m_map.begin();
	for( ; i != m_map.end(); ++i ) {
		if( i->second == rid ) {
			if( it )
				*it = i;
			return true;
		}
	}
	if( it )
		*it = m_map.end();
	return false;
}

const idmap::uid_type& idmap::GetUid(const rid_type &rid) const
{
	const_iterator i = m_map.begin();
	for( ; i != m_map.end(); ++i ) {
		if( i->second == rid )
			return i->first;
	}
	return m_blank_uid;
}

const idmap::rid_type& idmap::GetRid(const uid_type &uid) const
{
	const_iterator i = m_map.find(uid);
	return i->second;
}

const idmap::uid_type& idmap::operator[] (const rid_type &rid) const
{
	return GetUid(rid);
}

const idmap::rid_type& idmap::operator[] (const uid_type &uid) const
{
	return GetRid(uid);
}

// returns map::end() if either id already exists, otherwise
// returns newly mapped item iterator.
//
// The other versions of the function are to make conversion
// between different types easier, giving ability to map
// with any reasonable type, and then access the real
// values through the iterator if needed.
idmap::const_iterator idmap::Map(const uid_type &uid, const rid_type &rid)
{
	// neither id can be blank
	if( uid.size() == 0 || rid == 0 )
		return m_map.end();

	// neither id must already exist
	if( UidExists(uid) || RidExists(rid) )
		return m_map.end();

	return m_map.insert(m_map.begin(), make_pair(uid, rid));
}

/*
idmap::const_iterator idmap::Map(unsigned long uid_number, const rid_type &rid)
{
}

idmap::const_iterator idmap::Map(unsigned long uid_number, const std::string &rid_string)
{
}
*/

void idmap::UnmapUid(const uid_type &uid)
{
	m_map.erase(uid);
}

void idmap::UnmapRid(const rid_type &rid)
{
	iterator i = m_map.begin();
	for( ; i != m_map.end(); ++i ) {
		if( i->second == rid ) {
			m_map.erase(i);
			return;
		}
	}
}

