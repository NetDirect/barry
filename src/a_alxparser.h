///
/// \file	a_alxparser.h
///		ALX file parser (for one file)
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

#ifndef __BARRY_A_ALX_PARSER_H__
#define __BARRY_A_ALX_PARSER_H__

#include <vector>

#include "dll.h"
#include "xmlparser.h"
#include "a_codsection.h"
#include "a_library.h"
#include "a_application.h"
#include "a_osloader.h"
#include <tr1/memory>


namespace Barry {

namespace ALX {


class BXEXPORT ALXParser : public XML::XMLParser
{
public:
	enum MainNodeType {
		MAIN_NONE,
		IN_LOADER,
		IN_SYSTEM,
		IN_SYSTEM_APPLICATION,
		IN_SYSTEM_LIBRARY,
		IN_APPLICATION,
		IN_APPLICATION_APPLICATION,
		IN_LIBRARY
	};

	enum SubNodeType {
		SUB_NONE,
		IN_DIRECTORY,
		IN_OSFILES,
		IN_NAME,
		IN_DESCRIPTION,
		IN_VERSION,
		IN_VENDOR,
		IN_COPYRIGHT,
		IN_LANGUAGE,
		IN_LANGUAGE_SUPPORTED,
		IN_REQUIRED,
		IN_FILESET
	};

private:
	bool m_register;
	OSLoader& osloader;
	MainNodeType node;
	SubNodeType subnode;
	std::string buffdata;

	std::tr1::shared_ptr<CODSection> m_codsection;
	std::tr1::shared_ptr<CODSection> m_savecodsection;

public:
	ALXParser(OSLoader& osloader, std::istream& input);
	virtual ~ALXParser(void);

	virtual bool Run(const bool enable);

protected:
	// SaxParser overrides, also overridden in XMLParser
	virtual void on_start_document();
	virtual void on_end_document();
	virtual void on_start_element(const Glib::ustring& name,
		const xmlpp::SaxParser::AttributeList& attrs);
	virtual void on_end_element(const Glib::ustring& name);
	virtual void on_characters(const Glib::ustring& data);
	virtual void on_comment(const Glib::ustring& text);
	virtual void on_warning(const Glib::ustring& text);
	virtual void on_error(const Glib::ustring& text);
	virtual void on_fatal_error(const Glib::ustring& text);
};


} // namespace ALX

} // namespace Barry

#endif


