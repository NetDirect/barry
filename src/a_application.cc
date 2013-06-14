///
/// \file	a_application.cc
///		ALX Application class based on CODSection class
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

#include "i18n.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include "a_application.h"
#include "ios_state.h"


namespace Barry {


namespace ALX {


Application::Application(void)
	: CODSection()
{
}


Application::Application(const xmlpp::SaxParser::AttributeList& attrs)
	: CODSection(attrs)
{
}


Application::~Application(void)
{
}


void Application::Dump(std::ostream &os) const
{
	ios_format_state state(os);

	os << _("  Application ") << name << " - " << version << std::endl;
	os << _("    ID          : ") << id << std::endl;
	os << _("    Description : ") << description << std::endl;
	os << _("    Vendor      : ") << vendor << std::endl;
	os << _("    Copyright   : ") << copyright << std::endl;
	os << _("    Required    : ") << (isRequired ? _("Yes") : _("No")) << std::endl;

	std::vector<std::string>::const_iterator b = codfiles.begin(), e = codfiles.end();

	os << _("    Files       : ") << std::endl;
	for (; b != e; b++)
		os << "        - " << directory << "/" << (*b) << std::endl;
}


} // namespace ALX

} // namespace Barry

