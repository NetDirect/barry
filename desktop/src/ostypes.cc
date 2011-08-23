///
/// \file	ostypes.cc
///		Low level type helper functions for os wrapper library
///

/*
    Copyright (C) 2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "ostypes.h"
#include <iostream>

using namespace std;

namespace OpenSync { namespace Config {

pst_type PSTString2Type(const std::string &pst_string)
{
	pst_type types = PST_NONE;

	if( pst_string.find("contact") != string::npos )
		types |= PST_CONTACTS;
	if( pst_string.find("event") != string::npos )
		types |= PST_EVENTS;
	if( pst_string.find("note") != string::npos )
		types |= PST_NOTES;
	if( pst_string.find("todo") != string::npos )
		types |= PST_TODOS;
	if( pst_string.find("default_only") != string::npos )
		types |= PST_DO_NOT_SET;

cout << "pst_string '" << pst_string << "' to " << types << endl;
	return types;
}

std::string PSTType2String(pst_type types)
{
	string pst_string;

	if( types & PST_CONTACTS )   pst_string += "contact ";
	if( types & PST_EVENTS )     pst_string += "event ";
	if( types & PST_NOTES )      pst_string += "note ";
	if( types & PST_TODOS )      pst_string += "todo ";
	if( types & PST_DO_NOT_SET ) pst_string += "default_only ";

cout << "type " << types << " to '" << pst_string << "'" << endl;
	return pst_string;
}

}} // namespace OpenSync::Config

