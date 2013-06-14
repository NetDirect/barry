///
/// \file	boostwrap.cc
///		Wrap Boost serialization into its own lib, so each usage
///		in the tools doesn't cause a compile speed meltdown.
///

/*
    Copyright (C) 2012-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#include "boostwrap.h"
#include "i18n.h"

using namespace std;
using namespace Barry;

#ifdef __BARRY_BOOST_MODE__

template <class RecordT>
bool DoLoadBoostFile(const std::string &filename,
	std::vector<RecordT> &container,
	std::string &dbName,
	std::string &errmsg)
{
	try {
		std::ifstream ifs(filename.c_str());
		std::getline(ifs, dbName);
		boost::archive::text_iarchive ia(ifs);
		ia >> container;
		return true;
	}
	catch( boost::archive::archive_exception &ae ) {
		errmsg = _("Archive exception in DoLoadBoostFile(): ");
		errmsg += ae.what();
		return false;
	}
}

template <class RecordT>
bool DoSaveBoostFile(const std::string &filename,
	const std::vector<RecordT> &container,
	std::string &errmsg)
{
	try {
		std::ofstream ofs(filename.c_str());
		ofs << RecordT::GetDBName() << endl;
		boost::archive::text_oarchive oa(ofs);
		oa << container;
		return true;
	}
	catch( boost::archive::archive_exception &ae ) {
		errmsg = _("Archive exception in DoSaveBoostFile(): ");
		errmsg += ae.what();
		return false;
	}
}

#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
bool LoadBoostFile(const std::string &filename, \
	std::vector<tname> &container, \
	std::string &dbName, \
	std::string &errmsg) \
{ \
	return DoLoadBoostFile<tname>(filename, container, dbName, errmsg); \
} \
bool SaveBoostFile(const std::string &filename, \
	const std::vector<tname> &container, \
	std::string &errmsg) \
{ \
	return DoSaveBoostFile<tname>(filename, container, errmsg); \
}
ALL_KNOWN_PARSER_TYPES

#endif

