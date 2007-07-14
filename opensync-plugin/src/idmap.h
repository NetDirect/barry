//
// \file	uidmap.h
//		Class that maps opensync UID strings to Blackberry
//		Record ID's and back.
//

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_SYNC_UIDMAP_H__
#define __BARRY_SYNC_UIDMAP_H__

#include <map>
#include <string>
#include <stdint.h>

class idmap
{
public:
	typedef std::string			uid_type;
	typedef uint32_t			rid_type; // record ID
	typedef std::map<uid_type, rid_type>	map_type;
	typedef map_type::iterator		iterator;
	typedef map_type::const_iterator	const_iterator;

private:
	// blank objects, so references work
	uid_type m_blank_uid;
	rid_type m_blank_rid;

	// the map data
	map_type m_map;

public:
	idmap();
	~idmap();

	bool Load(const char *filename);
	bool Save(const char *filename) const;

	bool UidExists(const uid_type &uid, const_iterator *it = 0) const;
	bool RidExists(const rid_type &rid, const_iterator *it = 0) const;

	const uid_type& GetUid(const rid_type &rid) const;
	const rid_type& GetRid(const uid_type &uid) const;

	const uid_type& operator[] (const rid_type &rid) const;
	const rid_type& operator[] (const uid_type &uid) const;

	// returns map::end() if either id already exists, otherwise
	// returns newly mapped item iterator.
	//
	// The other versions of the function are to make conversion
	// between different types easier, giving ability to map
	// with any reasonable type, and then access the real
	// values through the iterator if needed.
	const_iterator Map(const uid_type &uid, const rid_type &rid);
	const_iterator Map(unsigned long uid_number, const rid_type &rid);
	const_iterator Map(unsigned long uid_number, const std::string &rid_string);

	void Unmap(iterator i) { m_map.erase(i); }
	void UnmapUid(const uid_type &uid);
	void UnmapRid(const rid_type &rid);

	// some stl-like functions
	iterator begin() { return m_map.begin(); }
	iterator end() { return m_map.end(); }
	void clear() { m_map.clear(); }
};

#endif

