///
/// \file	osbase.cc
///		Base API class helpers
///

/*
    Copyright (C) 2009, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "osbase.h"
#include "os22.h"
#include "os40.h"
#include <iostream>
#include <iomanip>
#include <barry/barry.h>

using namespace std;

std::ostream& operator<< (std::ostream &os, const string_list_type &list)
{
	string_list_type::const_iterator b = list.begin(), e = list.end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
	return os;
}

std::ostream& operator<< (std::ostream &os, const GroupMember &member)
{
	os << "Member ID: 0x" << hex << member.id
	   << "\n   Plugin Name: " << member.plugin_name;
	os << "\n   Friendly Name: ";
	if( member.friendly_name.size() )
		os << member.friendly_name;
	else
		os << "<not set>";
	return os;
}

std::ostream& operator<< (std::ostream &os, const member_list_type &list)
{
	member_list_type::const_iterator b = list.begin(), e = list.end();
	for( ; b != e; ++b ) {
		os << *b << endl;
	}
	return os;
}


/////////////////////////////////////////////////////////////////////////////
// OpenSyncAPISet public members

OpenSyncAPISet::OpenSyncAPISet()
{
}

OpenSyncAPISet::~OpenSyncAPISet()
{
	iterator b = begin(), e = end();
	for( ; b != e; ++b ) {
		delete *b;
	}

	base_type::clear();
}

// throws if not all can be opened
void OpenSyncAPISet::OpenAll()
{
	push_back( new OpenSync40 );
	push_back( new OpenSync22 );
}

// does not throw
int OpenSyncAPISet::OpenAvailable()
{
	int loaded = 0;

	try {
		OpenSyncAPI *p = new OpenSync40;
		push_back(p);
		loaded++;
	}
	catch( std::exception &e ) {
		barryverbose("Unable to load opensync 0.40: " << e.what());
		push_back(0);
	}

	try {
		OpenSyncAPI *p = new OpenSync22;
		push_back(p);
		loaded++;
	}
	catch( std::exception &e ) {
		barryverbose("Unable to load opensync 0.22: " << e.what());
		push_back(0);
	}

	return loaded;
}

OpenSyncAPI* OpenSyncAPISet::os40()
{
	return (*this)[0];
}

OpenSyncAPI* OpenSyncAPISet::os22()
{
	return (*this)[1];
}

