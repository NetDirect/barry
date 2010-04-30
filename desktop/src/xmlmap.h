///
/// \file	xmlmap.h
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

#ifndef __XMLMAP_H__
#define __XMLMAP_H__

#include <string>
#include <vector>
#include <iosfwd>
#include <libxml++/libxml++.h>
#include <tr1/memory>

class XmlNodeMapping;

class XmlNodeSummary
{
	xmlpp::Node *m_node;
	XmlNodeMapping *m_mapping;
	Glib::ustring m_name;
	Glib::ustring m_summary;

protected:
	void CreateSummary();
	void ProcessListKey(std::ostream &os, const Glib::ustring &name);
	void ProcessKey(std::ostream &os, const Glib::ustring &key);

public:
	XmlNodeSummary(xmlpp::Node *node, XmlNodeMapping *mapping);

	Glib::ustring Path() const { return m_node ? m_node->get_path(): Glib::ustring(); }
	const Glib::ustring& Name() const { return m_name; }
	const Glib::ustring& Summary() const { return m_summary; }

	Glib::ustring GetContent(const Glib::ustring &child_name) const;
	Glib::ustring GetContent(xmlpp::Node *node) const;

	static bool SummaryCompare(const XmlNodeSummary &a,
				const XmlNodeSummary &b);
	static bool PathCompare(const XmlNodeSummary &a,
				const XmlNodeSummary &b);
};

class XmlNodeMapping
{
public:
	typedef std::vector<Glib::ustring>		compare_list;
	typedef std::vector<XmlNodeSummary>		summary_list;

private:
	Glib::ustring m_node_key_name;	// "/contact/Address" excluding any
					// [1] and [2] offsets
					// This is is from the friendly map
					// definition file.
	int m_priority;
	Glib::ustring m_format;
	compare_list m_compare_node_names;
	summary_list m_summaries;

public:
	XmlNodeMapping(const Glib::ustring &node_name, int priority,
		const Glib::ustring &format);

	// data access
	const Glib::ustring& KeyName() const { return m_node_key_name; }
	Glib::ustring ShortName() const
		{ return size() ? m_summaries[0].Name() : Glib::ustring(); }
	int Priority() const { return m_priority; }
	const Glib::ustring& Format() const { return m_format; }
	bool IsEmpty() const { return size() == 0; }

	// operations
	void AddCompareName(const Glib::ustring &node_name);
	void AddNode(xmlpp::Node *node);
	void SortBySummary();
	void SortByPath();

	const XmlNodeSummary& operator[] (int index) const;
	size_t size() const { return m_summaries.size(); }

	bool operator== (const XmlNodeMapping &other) const;
	bool operator!= (const XmlNodeMapping &other) const
		{ return !operator==(other); }

	void Dump(std::ostream &os) const;
	void DumpSummaries(std::ostream &os) const;
};

class XmlNodeMap
	: public std::vector<std::tr1::shared_ptr<XmlNodeMapping> >
{
	// we use a vector of shared_ptr's here because the Summary
	// objects store pointers to the Mappings, and must remain stable
public:
	typedef std::tr1::shared_ptr<XmlNodeMapping>		ptr_type;
	typedef std::vector<ptr_type>				base_type;

	// helps to hide the shared_ptr complexity when the
	// user iterates through the vector
	template <class BaseT, class TargetT>
	class deref_iterator : public BaseT
	{
	public:
		explicit deref_iterator(BaseT it)
			: BaseT(it)
			{}
		TargetT& operator*() const
			{ return *BaseT::operator*(); }
		TargetT* operator->() const
			{ return &(*BaseT::operator*()); }
	};

	typedef deref_iterator<base_type::iterator, XmlNodeMapping> iterator;
	typedef deref_iterator<base_type::const_iterator, XmlNodeMapping> const_iterator;

private:
	void LoadMap(const std::string &type_name,
		const std::string &map_filename);

public:
	XmlNodeMap(const std::string &type_name,
		const std::string &map_filename);
	XmlNodeMap(const std::string &map_filename);

	void ImportNodes(xmlpp::Node *node, bool purge = false);
	void PurgeEmpties();
	XmlNodeMapping* Find(const Glib::ustring &node_name);

	void SortBySummary();
	void SortByPath();

	void Dump(std::ostream &os, int stop_priority = 1) const;

	iterator begin() { return iterator(base_type::begin()); }
	iterator end()   { return iterator(base_type::end()); }
	const_iterator begin() const { return const_iterator(base_type::begin()); }
	const_iterator end() const { return const_iterator(base_type::end()); }
	iterator priority_end(int stop_priority = 1);
};

#endif

