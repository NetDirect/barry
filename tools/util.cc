///
/// \file	util.cc
///		Misc. utility functions for command line tools.
///

/*
    Copyright (C) 2012, Net Direct Inc. (http://www.netdirect.ca/)

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
#include "i18n.h"

#include <barry/barry.h>
#ifdef __BARRY_SYNC_MODE__
#include <barry/barrysync.h>
#endif

#include <iostream>

using namespace std;
using namespace Barry;

void ShowParsers(bool show_fields, bool show_mime_support)
{
	cout << _("Supported Database parsers:\n");

#ifdef __BARRY_SYNC_MODE__
	if( show_mime_support )
		cout << _(" (* = can display in vformat MIME mode)\n");

#define MIME_DOT(tname) (MimeDump<tname>::Supported() && show_mime_support ? " *" : "")

#else
#define MIME_DOT(tname) ""
#endif

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
	{ \
		cout << "   " << tname::GetDBName() << MIME_DOT(tname) << "\n"; \
		if( show_fields ) { \
			cout << "      "; \
			FieldHandle<tname>::ListT::const_iterator \
					fhi = tname::GetFieldHandles().begin(), \
					fhe = tname::GetFieldHandles().end(); \
			for( int count = 0, len = 6; fhi != fhe; ++fhi, ++count ) { \
				if( count ) { \
					cout << ", "; \
					len += 2; \
				} \
				std::string name = fhi->GetIdentity().Name; \
				if( len + name.size() >= 75 ) { \
					cout << "\n      "; \
					len = 6; \
				} \
				cout << name; \
				len += name.size(); \
			} \
			cout << "\n"; \
		} \
	}

	ALL_KNOWN_PARSER_TYPES

	cout << endl;
}

void ShowBuilders()
{
	cout << _("Supported Database builders:\n");

#undef HANDLE_BUILDER
#define HANDLE_BUILDER(tname) cout << "   " << tname::GetDBName() << "\n";
	ALL_KNOWN_BUILDER_TYPES

	cout << endl;
}

