///
/// \file	util.cc
///		GUI utility functions
///

/*
    Copyright (C) 2007, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "util.h"

Glib::RefPtr<Gnome::Glade::Xml> LoadXml(const char *filename)
{
	// try loading from global glade directory first
	try {
		std::string file = BARRYBACKUP_GLADEDIR;
		file += filename;

		Glib::RefPtr<Gnome::Glade::Xml> ref = Gnome::Glade::Xml::create(file);
		return ref;
	}
	catch( Gnome::Glade::XmlError &e ) {
		// oops... let's fall through, and try the local directory
	}

	// this will throw XmlError on failure, and we let the caller worry
	// about that
	return Gnome::Glade::Xml::create(filename);
}

