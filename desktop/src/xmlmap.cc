///
/// \file	xmlmap.cc
///		Map an XML node tree according to an XML map file.
///		The map is not fully nested, and provides mostly a
///		flat view, based on the map.
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

#include "xmlmap.h"
#include <barry/tzwrapper.h>
#include <libxml++/libxml++.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <stdexcept>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
// XmlNodeSummary

XmlNodeSummary::XmlNodeSummary(xmlpp::Node *node, XmlNodeMapping *mapping)
	: m_node(node)
	, m_mapping(mapping)
	, m_name(node->get_name())
{
	CreateSummary();
}

void XmlNodeSummary::ProcessListKey(ostream &os, const Glib::ustring &name)
{
	xmlpp::Node::NodeList list = m_node->get_children(name);
	xmlpp::Node::NodeList::iterator i = list.begin();
	for( int count = 0; i != list.end(); ++i ) {
		Glib::ustring content = GetContent(*i);
		if( !content.size() )
			continue;

		if( count )
			os << ", ";
		os << content;
		count++;
	}
}

void XmlNodeSummary::ProcessKey(ostream &os, const Glib::ustring &key)
{
	if( key.size() == 0 )
		return;

	switch( key[0] )
	{
	case '(':
		os << "(";
		ProcessKey(os, key.substr(1));
		os << ") ";
		break;

	case '$': {
		bool comma = false;
		Glib::ustring wkey = key;
		if( key[key.size()-1] == ',' ) {
			comma = true;
			wkey = key.substr(0, key.size()-1);
		}

		Glib::ustring content = GetContent(wkey.substr(1));
		os << content;
		if( comma && content.size() )
			os << ",";
		os << " ";
		}
		break;

	case '%':
		ProcessListKey(os, key.substr(1));
		break;

	default:
		os << key << " ";
		break;
	}
}

void XmlNodeSummary::CreateSummary()
{
	ostringstream oss;
	istringstream iss(m_mapping->Format());
	Glib::ustring key;
	while( iss >> key ) {
		ProcessKey(oss, key);
	}
	m_summary = oss.str();
}

Glib::ustring XmlNodeSummary::GetContent(const Glib::ustring &child_name) const
{
	xmlpp::Node::NodeList list = m_node->get_children(child_name);
	if( list.size() ) {
		return GetContent(*list.begin());
	}
	else {
		return Glib::ustring();
	}
}

Glib::ustring XmlNodeSummary::GetContent(xmlpp::Node *node) const
{
	Glib::ustring content_value;

	xmlpp::ContentNode *content = dynamic_cast<xmlpp::ContentNode*>(node);
	while( !content ) {
		xmlpp::Node::NodeList list = node->get_children();
		if( !list.size() )
			break;

		node = *list.begin();
		content = dynamic_cast<xmlpp::ContentNode*>(node);
	}

	if( content ) {
		content_value = content->get_content();
	}
	return content_value;
}

bool XmlNodeSummary::SummaryCompare(const XmlNodeSummary &a,
					const XmlNodeSummary &b)
{
	return a.Summary() < b.Summary();
}

bool XmlNodeSummary::PathCompare(const XmlNodeSummary &a,
					const XmlNodeSummary &b)
{
	return a.Path() < b.Path();
}


//////////////////////////////////////////////////////////////////////////////
// XmlNodeMapping

XmlNodeMapping::XmlNodeMapping(const Glib::ustring &node_name,
				int priority,
				const Glib::ustring &format)
	: m_node_key_name(node_name)
	, m_priority(priority)
	, m_format(format)
{
}

void XmlNodeMapping::AddCompareName(const Glib::ustring &node_name)
{
	m_compare_node_names.push_back(node_name);
}

void XmlNodeMapping::AddNode(xmlpp::Node *node)
{
	XmlNodeSummary summary(node, this);
	m_summaries.push_back(summary);
}

void XmlNodeMapping::SortBySummary()
{
	sort(m_summaries.begin(), m_summaries.end(),
		&XmlNodeSummary::SummaryCompare);
}

void XmlNodeMapping::SortByPath()
{
	sort(m_summaries.begin(), m_summaries.end(),
		&XmlNodeSummary::PathCompare);
}

const XmlNodeSummary& XmlNodeMapping::operator[] (int index) const
{
	return m_summaries[index];
}

// Parse a compare_name in the format of "name[type]" into
// its component parts, without the brackets.
void XmlNodeMapping::SplitCompareName(const Glib::ustring &compare_name,
					Glib::ustring &name,
					Glib::ustring &type)
{
	size_t pos = compare_name.find('[');
	if( pos == Glib::ustring::npos ) {
		name = compare_name;
		type.clear();
	}
	else {
		name = compare_name.substr(0, pos);
		pos++;
		while( compare_name[pos] && compare_name[pos] != ']' ) {
			type += compare_name[pos];
			pos++;
		}
	}
}

bool Timestamp2Unix(const Glib::ustring &stamp, time_t &result)
{
	struct tm split;
	bool utc;
	if( !Barry::Sync::iso_to_tm(stamp.c_str(), &split, utc) )
		return false;

	split.tm_isdst = -1;

	if( utc )
		result = Barry::Sync::utc_mktime(&split);
	else
		result = mktime(&split);

	return result != (time_t)-1;
}

bool TimestampsEqual(const Glib::ustring &content1,
			const Glib::ustring &content2)
{
	time_t t1, t2;
	if( !Timestamp2Unix(content1, t1) || !Timestamp2Unix(content2, t2) ) {
		// could throw there, but this is just used to
		// pretty up the display, so defaulting to false
		// is fine
		return false;
	}
	return t1 == t2;
}

bool XmlNodeMapping::operator== (const XmlNodeMapping &other) const
{
	// make sure mapfile data are the same
	if( KeyName() != other.KeyName() ||
	    Priority() != other.Priority() ||
	    Format() != other.Format() ||
	    m_compare_node_names != other.m_compare_node_names )
		return false;

	// if the node counts are different, not equal
	if( m_summaries.size() != other.m_summaries.size() )
		return false;

	// cycle through all compare names, and compare the
	// values for those names from each node summary
	for( compare_list::const_iterator namei = m_compare_node_names.begin();
		namei != m_compare_node_names.end();
		++namei )
	{
		for( summary_list::const_iterator sumi = m_summaries.begin(),
			other_sumi = other.m_summaries.begin();
			sumi != m_summaries.end() &&
			other_sumi != other.m_summaries.end();
			++sumi, ++other_sumi )
		{
			Glib::ustring name, type;
			SplitCompareName(*namei, name, type);

			Glib::ustring
				content1 = sumi->GetContent(name),
				content2 = other_sumi->GetContent(name);

			if( type == "timestamp" ) {
				// compare content as timestamps
				if( !TimestampsEqual(content1, content2) )
					return false;
			}
			else {
				// compare as raw strings
				if( content1 != content2 )
					return false;
			}
		}
	}

	// all good!
	return true;
}

void XmlNodeMapping::Dump(std::ostream &os) const
{
	os << m_node_key_name << ": Priority " << m_priority
		<< ", Nodes: " << m_summaries.size() << "\n";
	os << "   Format: " << m_format << "\n";
	os << "   Compare Names (" << m_compare_node_names.size() << "):\n";
	os << "      ";
	for( compare_list::const_iterator i = m_compare_node_names.begin();
		i != m_compare_node_names.end();
		++i )
	{
		if( i != m_compare_node_names.begin() )
			os << ", ";
		os << (*i);
	}
	os << "\n";

	if( m_summaries.size() ) {
		os << "   Summaries (" << m_summaries.size() << " nodes):\n";
		for( summary_list::const_iterator i = m_summaries.begin();
			i != m_summaries.end();
			++i )
		{
			os << "      " << i->Name() << " > " << i->Summary() << "\n";
		}
	}

	os << "\n";
}

void XmlNodeMapping::DumpSummaries(std::ostream &os) const
{
	for( size_t n = 0; n < size(); n++ ) {
		os << m_summaries[n].Summary() << endl;
	}
}


//////////////////////////////////////////////////////////////////////////////
// XmlNodeMap

XmlNodeMap::XmlNodeMap(const std::string &type_name,
			const std::string &map_filename)
{
	LoadMap(type_name, map_filename);
}

XmlNodeMap::XmlNodeMap(const std::string &map_filename)
{
	LoadMap(string(), map_filename);
}

// copy constructor - only copies the map data, not the node.
// you must Import as usual
XmlNodeMap::XmlNodeMap(const XmlNodeMap &other)
{
	operator=(other);
}

void XmlNodeMap::LoadMap(const std::string &type_name,
			const std::string &map_filename)
{
	ifstream is(map_filename.c_str());
	if( !is )
		throw std::runtime_error("Unable to open: " + map_filename);

	std::string line;
	while( getline(is, line) ) {
		if( line[0] == '#' || line[0] == '\t' )
			continue;
		if( line.size() == 0 )
			continue;

		string type, key, format;
		int priority = -1;
		istringstream iss(line);
		iss >> type >> key >> priority >> std::ws;
		getline(iss, format);

		if( !iss ) {
			// skip if error
			continue;
		}

		if( type_name.size() && type != type_name ) {
			// skip types not for us
			continue;
		}

		ptr_type mapping( new XmlNodeMapping(key, priority, format) );

		while( is.peek() == '\t' ) {
			getline(is, line);
			mapping->AddCompareName(line.substr(1));
		}

		push_back(mapping);
	}
}

void XmlNodeMap::ImportNodes(xmlpp::Node *node, bool purge)
{
	if( XmlNodeMapping *mapping = Find(node->get_path()) ) {
		mapping->AddNode(node);
		return;
	}

	xmlpp::Node::NodeList list = node->get_children();
	xmlpp::Node::NodeList::iterator i = list.begin();
	for( ; i != list.end(); ++i ) {
		ImportNodes(*i, false);
	}

	if( purge ) {
		PurgeEmpties();
	}
}

/// Removes all XmlNodeMappings that contain no node summaries.
/// This lets the caller remove mappings that do not match the
/// XML data that was imported.  For example, a map may contain
/// mappings for Contacts and Calendar items, but only one
/// is parsed and imported at once.  This function will remove
/// the unused mappings.
void XmlNodeMap::PurgeEmpties()
{
//	iterator new_end = remove_if(begin(), end(),
//		mem_fun_ref(&XmlNodeMapping::IsEmpty));

	// -------------------------------------------------------
	// not all compilers compile remove_if() for us, so do it
	// manually: find the first empty
	iterator new_end = begin();
	for( ; new_end != end(); ++new_end ) {
		if( new_end->IsEmpty() )
			break;
	}
	if( new_end == end() )
		return; // nothing is empty, nothing to purge

	// copy the remainder, skipping the empties
	iterator good = new_end;
	++good;
	for( ; good != end(); ++good ) {
       		if( !good->IsEmpty() ) {
			*new_end = *good;
			++new_end;
		}
	}
	// done re-implementation of remove_if()
	// -------------------------------------------------------

	// erase the junk at the end (need to call this whether
	// you use remove_if() or not
	erase(new_end, end());
}

XmlNodeMapping* XmlNodeMap::Find(const Glib::ustring &node_name)
{
	iterator i = begin();
	for( ; i != end(); ++i ) {
		const Glib::ustring &xpath = node_name;
		XmlNodeMapping *mapping = &(*i);
		const Glib::ustring &key = mapping->KeyName();

		if( xpath.size() == key.size() && xpath == key ) {
			return mapping;
		}
		else if( xpath.size() > key.size() &&
			xpath.substr(0, key.size()) == key &&
			xpath[key.size()] == '[' )
		{
			return mapping;
		}
	}
	return 0;
}

void XmlNodeMap::SortBySummary()
{
	for_each(begin(), end(), mem_fun_ref(&XmlNodeMapping::SortBySummary));
}

void XmlNodeMap::SortByPath()
{
	for_each(begin(), end(), mem_fun_ref(&XmlNodeMapping::SortByPath));
}

void XmlNodeMap::Dump(std::ostream &os, int stop_priority) const
{
	const_iterator i = begin();
	for( ; i != end(); ++i ) {
		if( i->Priority() >= stop_priority )
			return;
		i->Dump(os);
	}
}

XmlNodeMap::iterator XmlNodeMap::priority_end(int stop_priority)
{
	iterator i = begin();
	for( ; i != end(); ++i ) {
		if( i->Priority() >= stop_priority )
			return i;
	}
	return i;
}

void XmlNodeMap::clear()
{
	for_each(begin(), end(), mem_fun_ref(&XmlNodeMapping::clear));
}

XmlNodeMap& XmlNodeMap::operator=(const XmlNodeMap &other)
{
	const_iterator i = other.begin();
	for( ; i != other.end(); ++i ) {
		ptr_type p( new XmlNodeMapping(*i) );
		p->clear();
		push_back(p);
	}
	return *this;
}

#ifdef XMLMAP
#include <locale>
#include <glibmm.h>
#include <tr1/functional>
using namespace std::tr1;
using namespace std::tr1::placeholders;

int main(int argc, char *argv[])
{
	try {
		locale loc("");
		cin.imbue( loc );
		cout.imbue( loc );

		xmlpp::DomParser parser;
		parser.parse_stream(cin);

		XmlNodeMap np("Contact", "0.22/xmlmap");
		np.ImportNodes(parser.get_document()->get_root_node());
		np.Dump(cout, 99);

		np.SortBySummary();
		cout << "\n\nCute summary:\n";
		for_each(np.begin(), np.priority_end(),
			bind( mem_fn(&XmlNodeMapping::DumpSummaries),
				_1, ref(cout)));

		np.SortByPath();
		cout << "\n\nCute summary:\n";
		for_each(np.begin(), np.priority_end(),
			bind( mem_fn(&XmlNodeMapping::DumpSummaries),
				_1, ref(cout)));
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

