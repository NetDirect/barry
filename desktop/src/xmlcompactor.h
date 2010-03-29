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
public:
	typedef Glib::ustring				string_type;
	typedef std::map<string_type, string_type>	base_map_type;
	typedef xmlpp::DomParser			base_xml_type;
	typedef base_map_type::iterator			iterator;
	typedef base_map_type::const_iterator		const_iterator;
	typedef bool (XmlCompactor::*content_handler)(xmlpp::ContentNode*);

private:
	Glib::ustring m_skip_prefix;
	Glib::ustring m_common_prefix;

protected:
	Glib::ustring HackPath(const Glib::ustring &path);
	bool WalkNodes(xmlpp::Node *node, content_handler handler);
	bool DoMap(xmlpp::ContentNode *content);
	bool ComparePrefix(xmlpp::ContentNode *content);

public:
	XmlCompactor();

	/// returns the common prefix of all entries, suitable for
	/// use as an argument to Map()
	Glib::ustring FindCommonPrefix();

	void Map(const Glib::ustring &skip);
	Glib::ustring Value(const Glib::ustring &key);

	void Dump(std::ostream &os) const;
};

std::ostream& operator<<(std::ostream &os, XmlCompactor &parser);

#endif

