///
/// \file	a_alxparser.cc
///		ALX file parser (for one file)
///

/*
    Copyright (C) 2010, Nicolas VIVIEN
    Copyright (C) 2005-2010, Net Direct Inc. (http://www.netdirect.ca/)

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
#include <fstream>
#include <sstream>

#include "a_alxparser.h"


namespace Barry {


namespace ALX {


ALXParser::ALXParser(OSLoader& osloader, std::istream& input) 
	: XML::XMLParser(input, "ISO-8859-1")
	, osloader(osloader)
{
	node = MAIN_NONE;
	subnode = SUB_NONE;
	m_register = true;
}


ALXParser::~ALXParser(void)
{
}


bool ALXParser::Run(const bool enable)
{
	m_register = enable;

	return XMLParser::Run();
}


void ALXParser::on_start_document()
{
//	std::cout << "on_start_document()" << std::endl;
}


void ALXParser::on_end_document()
{
//	std::cout << "on_end_document()" << std::endl;
}


void ALXParser::on_start_element(const Glib::ustring& name,
	const xmlpp::SaxParser::AttributeList& attrs)
{
	depth++;

	buffdata = "";

	switch (node) {
	case MAIN_NONE:
		if (name == "loader")
			node = IN_LOADER;
//		else
//			exit(-1);
		break;

	case IN_LOADER:
		subnode = SUB_NONE;

		if (name == "system") {
			node = IN_SYSTEM;
		}
		else if (name == "application") {
			node = IN_APPLICATION;
	
			m_codsection = new Application(attrs);
		}
		else if (name == "library") {
			node = IN_LIBRARY;

			m_codsection = new Library(attrs);
		}
		break;

	case IN_SYSTEM:
		if (name == "directory")
			subnode = IN_DIRECTORY;
		else if (name == "osfiles") {
			subnode = IN_OSFILES;
		}
		else if (name == "application") {
			node = IN_SYSTEM_APPLICATION;
			subnode = SUB_NONE;

			m_codsection = new Application();
		}
		else if (name == "library") {
			node = IN_SYSTEM_LIBRARY;
			subnode = SUB_NONE;
		}
		else if ((subnode == IN_OSFILES) && (name == "os")) 
			osloader.AddProperties(attrs);
		break;

	case IN_LIBRARY:
	case IN_APPLICATION:
	case IN_SYSTEM_APPLICATION:
		if (subnode == SUB_NONE) {
			if (name == "name")
				subnode = IN_NAME;
			else if (name == "description")
				subnode = IN_DESCRIPTION;
			else if (name == "version")
				subnode = IN_VERSION;
			else if (name == "vendor")
				subnode = IN_VENDOR;
			else if (name == "copyright")
				subnode = IN_COPYRIGHT;
			else if (name == "directory") {
				if (osloader.IsSupported(attrs))
					subnode = IN_DIRECTORY;
			}
			else if (name == "language") {
				if (osloader.IsSupported(attrs))
					subnode = IN_LANGUAGE_SUPPORTED;
				else
					subnode = IN_LANGUAGE;
			}
			else if (name == "required")
				subnode = IN_REQUIRED;
			else if (name == "fileset") {
				if (osloader.IsSupported(attrs))
					subnode = IN_FILESET;
			}
		}
		break;

	default:
		break;
	}
}


void ALXParser::on_end_element(const Glib::ustring& name)
{
	depth--;

	switch (node) {
	case MAIN_NONE:
//		exit(-1);
		break;

	case IN_LOADER:
		if (name == "loader")
			node = MAIN_NONE;
		break;

	case IN_SYSTEM:
		if (name == "system") {
			subnode = SUB_NONE;
			node = IN_LOADER;
		}

		switch (subnode) {
		case IN_DIRECTORY:
			if (name == "directory")
				subnode = SUB_NONE;
			break;

		case IN_OSFILES:
			if (name == "osfiles")
				subnode = SUB_NONE;
			else if (name == "os")
				osloader.SetSFIFile(buffdata);
			break;
		default:
			break;
		}
		break;

	case IN_LIBRARY:
	case IN_APPLICATION:
	case IN_SYSTEM_APPLICATION:
		if (name == "application") {
			if (m_register)
				osloader.AddApplication(m_codsection);
			subnode = SUB_NONE;
			if (node == IN_APPLICATION)
				node = IN_LOADER;
			else if (node == IN_SYSTEM_APPLICATION)
				node = IN_SYSTEM;
		}
		else if (name == "library") {
			if (m_register)
				osloader.AddLibrary(m_codsection);
			subnode = SUB_NONE;
			if (node == IN_LIBRARY)
				node = IN_LOADER;
		}

		switch (subnode) {
		case IN_NAME:
			if (name == "name") {
				m_codsection->SetName(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_DESCRIPTION:
			if (name == "description") {
				m_codsection->SetDescription(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_VERSION:
			if (name == "version") {
				m_codsection->SetVersion(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_VENDOR:
			if (name == "vendor") {
				m_codsection->SetVendor(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_COPYRIGHT:
			if (name == "copyright") {
				m_codsection->SetCopyright(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_DIRECTORY:
			if (name == "directory") {
				m_codsection->SetDirectory(buffdata);
				subnode = SUB_NONE;
			}
			break;
		case IN_LANGUAGE:
			if (name == "language") {
				subnode = SUB_NONE;
			}
			break;
		case IN_LANGUAGE_SUPPORTED:
			if (name == "language") {
				subnode = SUB_NONE;
			}
			else if (name == "name") {
				m_codsection->SetName(buffdata);
			}
			break;
		case IN_REQUIRED:
			if (name == "required") {
				subnode = SUB_NONE;
				m_codsection->SetRequired(buffdata);
			}
			break;
		case IN_FILESET:
			if (name == "fileset") {
				subnode = SUB_NONE;
			}
			else if (name == "files") {
				m_codsection->AddFiles(buffdata);
			}
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
}


void ALXParser::on_characters(const Glib::ustring& data)
{
	buffdata.append(data);
}


void ALXParser::on_comment(const Glib::ustring& text)
{
//	std::cout << "on_comment(): " << text << std::endl;
}


void ALXParser::on_warning(const Glib::ustring& text)
{
//	std::cout << "on_warning(): " << text << std::endl;
}


void ALXParser::on_error(const Glib::ustring& text)
{
//	std::cout << "on_error(): " << text << std::endl;
}


void ALXParser::on_fatal_error(const Glib::ustring& text)
{
	std::cout << "on_fatal_error(): " << text << std::endl;
}


} // namespace ALX

} // namespace Barry

