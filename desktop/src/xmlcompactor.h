///
/// \file	xmlcompactor.h
///		Compact an XML file into a map of pretty xpaths and content
///

/*
    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>

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

#ifndef __XMLCOMPACTOR_H__
#define __XMLCOMPACTOR_H__

#include <libxml++/libxml++.h>
#include <map>
#include <iosfwd>

class XmlCompactor
	: public std::map<Glib::ustring, Glib::ustring>
	, public xmlpp::DomParser
{
	Glib::ustring m_skip_prefix;

protected:
	void DoMap(xmlpp::Node *node);
	Glib::ustring HackPath(const Glib::ustring &path);

public:
	XmlCompactor(const Glib::ustring &skip);

	void Map();

	void Dump(std::ostream &os) const;
};

std::ostream& operator<<(std::ostream &os, XmlCompactor &parser);

#endif

