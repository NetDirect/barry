///
/// \file	ldifio.h
///		Storage, parser, and builder classes for ldif operations.
///

/*
    Copyright (C) 2010-2012, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRY_LDIFIO_H__
#define __BARRY_LDIFIO_H__

#include "dll.h"
#include "ldif.h"
#include <memory>
#include <iostream>
#include <fstream>

namespace Barry {

class Builder;

//
// LdifStore
//
/// Storage class suitable for use in a RecordParser<> or RecordBuilder<>.
/// Be sure to use the input constructors for RecordBuilder<> objects
/// and output constructors for RecordParser<> objects.
///
/// Examples:
///	Read contacts from an ldif stream on stdin:
///	new RecordBuilder<Contact, LdifStore>( new LdifStore(cin) );
///
///	Write contacts to an ldif stream on stdout:
///	new RecordParser<Contact, LdifStore>(
///		new LdifStore(cout, baseDN, dnAttr) );
///
class BXEXPORT LdifStore
{
	std::auto_ptr<std::ifstream> m_ifs;
	std::auto_ptr<std::ofstream> m_ofs;
	std::istream &m_is;
	std::ostream &m_os;
	bool m_end_of_file;

	Barry::ContactLdif m_ldif;

public:
	// input constructors
	LdifStore(const std::string &filename);
	LdifStore(std::istream &is);

	// output constructors
	LdifStore(const std::string &filename, const std::string &baseDN,
		const std::string &dnattr);
	LdifStore(std::ostream &os, const std::string &baseDN,
		const std::string &dnattr);

	// storage operator
	void operator() (const Contact &rec);

	// retrieval operator
	bool operator() (Contact &rec, const Builder &builder);
};

}

#endif

