///
/// \file	xmlcompactor.cc
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

#include "xmlcompactor.h"
#include <glibmm.h>
#include <iostream>
#include <locale>
using namespace std;

std::ostream& operator<<(std::ostream &os, XmlCompactor &parser)
{
	parser.Dump(os);
	return os;
}

XmlCompactor::XmlCompactor(const Glib::ustring &skip)
	: m_skip_prefix(skip)
{
}

// ugly hack to pretty up the output
Glib::ustring XmlCompactor::HackPath(const Glib::ustring &path)
{
	const char *bad[] = {	"[0]", "[1]", "[2]", "[3]", "[4]",
				"[5]", "[6]", "[7]", "[8]", "[9]" };
	const char *good[] = {	"[00]", "[01]", "[02]", "[03]", "[04]",
				"[05]", "[06]", "[07]", "[08]", "[09]" };

	Glib::ustring p = path;

	// strip off the ending "text()"
	size_t pos = p.rfind("/text()");
	if( pos != Glib::ustring::npos && (pos + 7) == p.size() )
		p.resize(pos);

	// remove leading name if possible
	if( m_skip_prefix.size() && p.find(m_skip_prefix) == 0 )
		p.replace(0, m_skip_prefix.size(), "");

	// remove leading slash
	if( p[0] == '/' )
		p.replace(0, 1, "");

	// convert single digit offsets to two
	for( int i = 0; i < 10; i++ ) {
		size_t pos;
		while( (pos = p.find(bad[i])) != Glib::ustring::npos ) {
			p.replace(pos, 3, good[i]);
		}
	}

	return p;
}

void XmlCompactor::DoMap(xmlpp::Node *node)
{
	xmlpp::ContentNode *content = dynamic_cast<xmlpp::ContentNode*>(node);
	if( content ) {
		if( content->is_white_space() )
			return;	// skip whitespace between content
		(*this)[HackPath(content->get_path())] = content->get_content();
	}

	xmlpp::Node::NodeList list = node->get_children();
	xmlpp::Node::NodeList::iterator i = list.begin();
	for( ; i != list.end(); ++i ) {
		DoMap(*i);
	}
}

void XmlCompactor::Map()
{
	DoMap(get_document()->get_root_node());
}

Glib::ustring XmlCompactor::Value(const Glib::ustring &key)
{
	iterator i = find(key);
	if( i == end() )
		return "";
	return i->second;
}

void XmlCompactor::Dump(std::ostream &os) const
{
	for( const_iterator i = begin(); i != end(); ++i ) {
		os << i->first.raw() << ": " << i->second.raw() << "\n";
	}
}


#ifdef XMLCOMPACTOR
int main(int argc, char *argv[])
{
	try {
		locale loc("");
		cin.imbue( loc );
		cout.imbue( loc );
		XmlCompactor parser(argc >= 2 ? argv[1] : "");
		parser.parse_stream(cin);
		parser.Map();
		cout << parser << endl;
	}
	catch( Glib::ConvertError &e ) {
		cerr << e.what() << endl;
		return 1;
	}
	catch( std::exception &e ) {
		cerr << e.what() << endl;
		return 1;
	}
}
#endif

