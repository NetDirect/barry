///
/// \file	xmlparser.cc
///		A simple XML parser (use callback on start, end and data block
///

/*
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2005-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include <iostream>

#include "xmlparser.h"


namespace Barry {

namespace XML {


XMLParser::XMLParser(std::istream& input, const char *charset)
	: xmlpp::SaxParser()
	, input(input)
{
	this->depth = 0;
	this->charset = charset;
}


XMLParser::~XMLParser(void)
{
}


const unsigned long XMLParser::GetDepth(void) const
{
	return depth;
}


bool XMLParser::Run(void)
{
	try {
		set_substitute_entities(true);
		parse_chunk("<?xml version=\"1.0\" encoding=\"" + charset + "\"?>");

		std::string line;
		while( getline(input, line) ) {
			parse_chunk(line);
		}
		finish_chunk_parsing();
	}
	catch (const xmlpp::exception& ex) {
		std::cout << "libxml++ exception: " << ex.what() << std::endl;
		return false;
	}

	return true;
}


void XMLParser::on_start_document()
{
	std::cout << "on_start_document()" << std::endl;
}


void XMLParser::on_end_document()
{
	std::cout << "on_end_document()" << std::endl;
}


void XMLParser::on_start_element(const Glib::ustring& name,
	const xmlpp::SaxParser::AttributeList& attributes)
{
	std::cout << "Start:" << name << std::endl;
	depth++;

	// Print attributes:
	for (xmlpp::SaxParser::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
		std::cout << "  Attribute name=" << iter->name << std::endl;

		std::cout << "    , value= " << iter->value << std::endl;
	}
}


void XMLParser::on_end_element(const Glib::ustring& name)
{
	std::cout << "End:" << name << std::endl;
	depth--;
}


void XMLParser::on_characters(const Glib::ustring& text)
{
	std::cout << "  Data:" << text << std::endl;
}


void XMLParser::on_comment(const Glib::ustring& text)
{
	std::cout << "on_comment(): " << text << std::endl;
}


void XMLParser::on_warning(const Glib::ustring& text)
{
	std::cout << "on_warning(): " << text << std::endl;
}


void XMLParser::on_error(const Glib::ustring& text)
{
	std::cout << "on_error(): " << text << std::endl;
}


void XMLParser::on_fatal_error(const Glib::ustring& text)
{
	std::cout << "on_fatal_error(): " << text << std::endl;
}


} // namespace XML

} // namespace Barry

