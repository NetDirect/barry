///
/// \file	ldifio.cc
///		Storage, parser, and builder classes for ldif operations.
///

/*
    Copyright (C) 2010-2011, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "ldifio.h"

namespace Barry {

LdifStore::LdifStore(const std::string &filename)
	: m_ifs( new std::ifstream(filename.c_str()) )
	, m_is(*m_ifs)
	, m_os(*m_ofs)	// yes, this is a reference to a null ptr
			// but will never be used (see below as well)
	, m_end_of_file(false)
	, m_ldif("")
{
}

LdifStore::LdifStore(std::istream &is)
	: m_is(is)
	, m_os(*m_ofs)
	, m_end_of_file(false)
	, m_ldif("")
{
}

// output constructors
LdifStore::LdifStore(const std::string &filename,
		const std::string &baseDN,
		const std::string &dnattr)
	: m_ofs( new std::ofstream(filename.c_str()) )
	, m_is(*m_ifs)
	, m_os(*m_ofs)
	, m_end_of_file(false)
	, m_ldif(baseDN)
{
	m_ldif.SetDNAttr(dnattr);
}

LdifStore::LdifStore(std::ostream &os,
		const std::string &baseDN,
		const std::string &dnattr)
	: m_is(*m_ifs)
	, m_os(os)
	, m_end_of_file(false)
	, m_ldif(baseDN)
{
	m_ldif.SetDNAttr(dnattr);
}

// storage operator
void LdifStore::operator() (const Contact &rec)
{
	m_ldif.DumpLdif(m_os, rec);
}

// retrieval operator
bool LdifStore::operator() (Contact &rec, const Barry::Builder &builder)
{
	if( m_end_of_file )
		return false;

	// there may be LDIF records in the input that generate
	// invalid Contact records, but valid Contact records
	// may come after.. so keep processing until end of stream
	while( m_is ) {
		if( m_ldif.ReadLdif(m_is, rec) )
			return true;
	}

	m_end_of_file = true;
	return false;
}

} // namespace Barry

