///
/// \file	xmlparser.h
///		A simple XML parser (use callback on start, end and data block
///

/*
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2005-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__


#include <iostream>

#include "dll.h"
#include "a_common.h"

namespace Barry {

namespace XML {


class BXEXPORT XMLParser : public xmlpp::SaxParser
{
public:
	XMLParser(std::istream& input, const char *charset="UTF-8");
	virtual ~XMLParser(void);

	virtual bool Run(void);
	virtual const unsigned long GetDepth(void) const;

protected:
	std::string charset;
	std::istream& input;
	unsigned long depth;

	// SaxParser overrides:
	virtual void on_start_document();
	virtual void on_end_document();
	virtual void on_start_element(const Glib::ustring& name,
		const xmlpp::SaxParser::AttributeList& properties);
	virtual void on_end_element(const Glib::ustring& name);
	virtual void on_characters(const Glib::ustring& characters);
	virtual void on_comment(const Glib::ustring& text);
	virtual void on_warning(const Glib::ustring& text);
	virtual void on_error(const Glib::ustring& text);
	virtual void on_fatal_error(const Glib::ustring& text);
};


} // namespace XML

} // namespace Barry

#endif

